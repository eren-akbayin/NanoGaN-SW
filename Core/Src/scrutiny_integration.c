#include "scrutiny_integration.h"
#include "cmsis_os.h"
#include "main.h"
#include "scrutiny_cwrapper.h"

/* ── Memory for C++ objects (sized at runtime by the library) ─────────────
   We over-allocate with a generous static buffer. The construct() calls
   will fail-safe if the buffer is too small.                               */
#define MAIN_HANDLER_BUF_SIZE 2048U
#define CONFIG_BUF_SIZE 1024U

static uint8_t main_handler_mem[MAIN_HANDLER_BUF_SIZE];
static uint8_t config_mem[CONFIG_BUF_SIZE];

static scrutiny_c_main_handler_t *s_handler = NULL;

/* ── Loop handler memory ───────────────────────────────────────────────── */
#define LOOP_HANDLER_BUF_SIZE 512U
static uint8_t loop_handler_mem[LOOP_HANDLER_BUF_SIZE];
static scrutiny_c_loop_handler_ff_t *s_loop_handler = NULL;

/* DAQ loop, driven from TIM4 @ 10kHz, feeds the embedded graph feature */
static uint8_t daq_loop_handler_mem[LOOP_HANDLER_BUF_SIZE];
static scrutiny_c_loop_handler_ff_t *s_daq_loop_handler = NULL;

static scrutiny_c_loop_handler_t *s_loop_array[2];

/* ── Comm buffers ──────────────────────────────────────────────────────── */
#define RX_BUF_SIZE 256U
#define TX_BUF_SIZE 256U

static uint8_t rx_buffer[RX_BUF_SIZE];
static uint8_t tx_buffer[TX_BUF_SIZE];

/* ── Datalogging buffer ────────────────────────────────────────────────── */
#if SCRUTINY_ENABLE_DATALOGGING == 1
/* Placed in RAM_D2 (32KB, 0x30000000) — DMA1/DMA2/MDMA accessible.
   32KB → 2048 samples/signal for 4 × float32 at 100kHz (~20ms).
   Requires SCRUTINY_DATALOGGING_BUFFER_32BITS=ON in CMake. */
#define DATALOG_BUF_SIZE (32U * 1024U)
static uint8_t datalog_buffer[DATALOG_BUF_SIZE] __attribute__((section(".d2_bss")));
#endif

/* ── Timestamp (FreeRTOS tick @ 1kHz → ×10 = 100µs = 1000 × 100ns) ─────  */
static uint32_t s_last_tick = 0;

static scrutiny_c_timediff_t get_timestep_100ns(void)
{
    uint32_t now = osKernelGetTickCount(); /* ms ticks */
    uint32_t delta = now - s_last_tick;    /* ms elapsed */
    s_last_tick = now;
    return (scrutiny_c_timediff_t)(delta * 10000U); /* ms → 100ns units */
}

/* ── Init ──────────────────────────────────────────────────────────────── */
void scrutiny_integration_init(void)
{
    scrutiny_c_config_t *config =
        scrutiny_c_config_construct(config_mem, sizeof(config_mem));

    scrutiny_c_config_set_buffers(config, rx_buffer, RX_BUF_SIZE, tx_buffer,
                                  TX_BUF_SIZE);

    scrutiny_c_config_set_display_name(config, "NanoGaN");
    scrutiny_c_config_set_max_bitrate(config, 0); /* unlimited */
    scrutiny_c_config_set_session_counter_seed(config, 0x1234U);
    scrutiny_c_config_memory_write_enable(config, 1);

#if SCRUTINY_ENABLE_DATALOGGING == 1
    scrutiny_c_config_set_datalogging_buffers(config, datalog_buffer,
                                              DATALOG_BUF_SIZE);
#endif

    s_handler = scrutiny_c_main_handler_construct(main_handler_mem,
                                                  sizeof(main_handler_mem));

    /* 10ms fixed frequency loop = 100Hz sampling rate
      timestep expressed in 100ns units: 10ms = 100,000 × 100ns */
    s_loop_handler = scrutiny_c_loop_handler_fixed_freq_construct(
        loop_handler_mem, LOOP_HANDLER_BUF_SIZE,
        100U, /* 10us in 100ns units → 100kHz */
        "main_loop");

    /* 100us fixed frequency loop = 10kHz sampling rate, driven by TIM4
      timestep expressed in 100ns units: 100us = 1,000 × 100ns */
    s_daq_loop_handler = scrutiny_c_loop_handler_fixed_freq_construct(
        daq_loop_handler_mem, LOOP_HANDLER_BUF_SIZE,
        1000U, /* 100us in 100ns units → 10kHz */
        "daq_loop");

    s_loop_array[0] = (scrutiny_c_loop_handler_t *)s_loop_handler;
    s_loop_array[1] = (scrutiny_c_loop_handler_t *)s_daq_loop_handler;
    scrutiny_c_config_set_loops(config, s_loop_array, 2);

    scrutiny_c_main_handler_init(s_handler, config);

    s_last_tick = osKernelGetTickCount();
}

/* ── Called from CDC RX callback ──────────────────────────────────────── */
void scrutiny_receive_data(const uint8_t *data, uint16_t len)
{
    if (s_handler != NULL) {
        scrutiny_c_main_handler_receive_data(s_handler, data, len);
    }
}

/* ── Process + collect output data ───────────────────────────────────── */
uint16_t scrutiny_process_and_collect(uint8_t *out_buf, uint16_t out_buf_size)
{
    if (s_handler == NULL) {
        return 0;
    }
    scrutiny_c_timediff_t timestep = get_timestep_100ns();
    scrutiny_c_main_handler_process(s_handler, timestep);

    uint16_t total = 0;
    uint16_t n;
    while ((n = scrutiny_c_main_handler_pop_data(s_handler, out_buf + total, out_buf_size - total)) > 0)
    {
        total += n;
        if (total >= out_buf_size) break;
    }
    return total;
}

void scrutiny_loop_process(uint32_t timestep_100ns)
{
    if (s_loop_handler == NULL) {
        return;
    }
    scrutiny_c_loop_handler_fixed_freq_process(s_loop_handler, timestep_100ns);
}

void scrutiny_daq_loop_process(uint32_t timestep_100ns)
{
    if (s_daq_loop_handler == NULL) {
        return;
    }
    scrutiny_c_loop_handler_fixed_freq_process(s_daq_loop_handler, timestep_100ns);
}