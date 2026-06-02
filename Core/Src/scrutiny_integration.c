#include "scrutiny_integration.h"
#include "cmsis_os.h"
#include "main.h"
#include "scrutiny_cwrapper.h"
#include "tusb.h"

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
static scrutiny_c_loop_handler_t *s_loop_array[1];

/* ── Comm buffers ──────────────────────────────────────────────────────── */
#define RX_BUF_SIZE 256U
#define TX_BUF_SIZE 256U

static uint8_t rx_buffer[RX_BUF_SIZE];
static uint8_t tx_buffer[TX_BUF_SIZE];

/* ── Datalogging buffer ────────────────────────────────────────────────── */
#if SCRUTINY_ENABLE_DATALOGGING == 1
#define DATALOG_BUF_SIZE 2048U
static uint8_t datalog_buffer[DATALOG_BUF_SIZE];
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
        100000U, /* 10ms in 100ns units → 100Hz */
        "main_loop");

    s_loop_array[0] = (scrutiny_c_loop_handler_t *)s_loop_handler;
    scrutiny_c_config_set_loops(config, s_loop_array, 1);

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

/* ── Process + TX flush — call this from your CDC task loop ───────────── */
void scrutiny_process_and_send(void)
{
    if (s_handler == NULL) {
        return;
    }

    scrutiny_c_timediff_t timestep = get_timestep_100ns();
    scrutiny_c_main_handler_process(s_handler, timestep);

    uint8_t out[64];
    uint16_t n;
    while ((n = scrutiny_c_main_handler_pop_data(s_handler, out, sizeof(out))) >0) 
    {
        tud_cdc_write(out, n);
    }
    tud_cdc_write_flush();
}

void scrutiny_loop_process(uint32_t timestep_100ns)
{
    if (s_loop_handler == NULL) {
        return;
    }
    scrutiny_c_loop_handler_fixed_freq_process(s_loop_handler, timestep_100ns);
}