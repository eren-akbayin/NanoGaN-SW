#include <string.h>
#include "core_tasks.h"
#include "cmsis_os.h"
#include "main.h"
#include "tusb.h"
#include "scrutiny_integration.h"
#include "measurement.h"

osThreadId_t usbDeviceTaskHandle;
const osThreadAttr_t usb_device_task_attributes = {
    .name = "usbDeviceTask",
    .stack_size = 1024 * 4,
    .priority = osPriorityHigh,
};

void usbDeviceTask(void *param)
{
    while (1) {
        tud_task();
    }
}

osThreadId_t scrutinyTaskHandle;
const osThreadAttr_t scrutinyTask_attributes = {
    .name = "scrutinyTask",
    .stack_size = 512 * 4,
    .priority = osPriorityNormal,
};


// Scrutiny uses tusb's CDC interface for communication
void scrutinyTask(void *param)
{

    scrutiny_integration_init();

    while (1) {
        /* RX */
        uint8_t rx_buf[64];
        uint32_t count = tud_cdc_n_read(SCRUTINY_CDC, rx_buf, sizeof(rx_buf));
        if (count > 0) {
            scrutiny_receive_data(rx_buf, (uint16_t)count);
        }

        /* TX */
        uint8_t tx_buf[256];
        uint16_t tx_len = scrutiny_process_and_collect(tx_buf, sizeof(tx_buf));
        if (tx_len > 0) {
            tud_cdc_n_write(SCRUTINY_CDC, tx_buf, tx_len);
        }
        tud_cdc_n_write_flush(SCRUTINY_CDC);

        osThreadYield();
    }
}

osThreadId_t userCDCTaskHandle;
const osThreadAttr_t userCDC_task_attributes = {
    .name = "userCDCTask",
    .stack_size = 256 * 4,
    .priority = osPriorityNormal,
};

static void cdc_reply(const char *str)
{
    tud_cdc_n_write(USER_CDC, (const uint8_t *)str, strlen(str));
    tud_cdc_n_write_flush(USER_CDC);
}

void userCDCTask(void *param)
{
    while (1) {
        uint8_t rx_buf[64];
        uint32_t count = tud_cdc_n_read(USER_CDC, rx_buf, sizeof(rx_buf));
        if (count > 0) {
            rx_buf[count < sizeof(rx_buf) ? count : sizeof(rx_buf) - 1] = '\0';

            if (strncmp((char *)rx_buf, "active", 6) == 0) {
                inverter_fsm_post_event(INV_EVT_START);
                cdc_reply("-> ACTIVE\r\n");
            } else if (strncmp((char *)rx_buf, "standby", 7) == 0) {
                inverter_fsm_post_event(INV_EVT_STOP);
                cdc_reply("-> STANDBY\r\n");
            } else if (strncmp((char *)rx_buf, "reset", 5) == 0) {
                inverter_fsm_post_event(INV_EVT_RESET);
                cdc_reply("-> RESET\r\n");
            } else if (strncmp((char *)rx_buf, "state", 5) == 0) {
                static const char *state_names[] = { "INIT", "STANDBY", "ACTIVE", "RESET", "FAULT" };
                cdc_reply(state_names[inverter_fsm_get_state()]);
                cdc_reply("\r\n");
            } else {
                cdc_reply("unknown command\r\n");
            }
        }
        osThreadYield();
    }
}

/* ── Inverter FSM task ────────────────────────────────────────────────────── */

static osMessageQueueId_t s_fsmQueue;
static volatile InverterState_t s_fsmState = INV_STATE_INIT;

void inverter_fsm_post_event(InverterEvent_t evt)
{
    InverterFsmMsg_t msg = { .event = evt, .faultSource = NONE };
    osMessageQueuePut(s_fsmQueue, &msg, 0, 0);
}

void inverter_fsm_post_fault(measurementType_t source)
{
    InverterFsmMsg_t msg = { .event = INV_EVT_FAULT, .faultSource = source };
    osMessageQueuePut(s_fsmQueue, &msg, 0, 0);
}

InverterState_t inverter_fsm_get_state(void)
{
    return s_fsmState;
}

static void fsm_enter_init(void)
{
    HAL_GPIO_WritePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port,  LED_FAULT_Pin,  GPIO_PIN_RESET);

    /* TODO: set real limits for your hardware */
    calibrateSensorsSetShutdowns(3.0f, 11.0f, 15.0f);
	HAL_TIM_Base_Start_IT(&htim1);

    inverter_fsm_post_event(INV_EVT_INIT_DONE);
}

static void fsm_enter_standby(void)
{
    stopGateDrive();
    startMeasurements();
    HAL_GPIO_WritePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port,  LED_FAULT_Pin,  GPIO_PIN_RESET);
}

static void fsm_enter_active(void)
{
    startMeasurements();
    HAL_GPIO_WritePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port,  LED_FAULT_Pin,  GPIO_PIN_RESET);
    
    /* Load duty cycles and start PWM */
    startGateDrive();

}

static void fsm_enter_fault(measurementType_t source)
{
    shutdownGateDrive(); /* ISR already did this, belt-and-suspenders for non-ISR faults */
    stopMeasurements();
    getShutdownInfo(source);
    HAL_GPIO_WritePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port,  LED_FAULT_Pin,  GPIO_PIN_SET);
}

static void fsm_enter_reset(void)
{
    HAL_GPIO_WritePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port,  LED_FAULT_Pin,  GPIO_PIN_RESET);
    /* TODO: clear fault flags, run self-check, then post INV_EVT_RESET_DONE */
    clearShutdownInfo();
    inverter_fsm_post_event(INV_EVT_RESET_DONE); /* placeholder – replace with real check */
    HAL_TIM_Base_Start(&htim2);

}

osThreadId_t inverterFsmTaskHandle;
const osThreadAttr_t inverterFsmTask_attributes = {
    .name       = "inverterFsmTask",
    .stack_size = 256 * 4,
    .priority   = osPriorityAboveNormal,
};

void inverterFsmTask(void *param)
{
    fsm_enter_init();

    while (1) {
        InverterFsmMsg_t msg;
        if (osMessageQueueGet(s_fsmQueue, &msg, NULL, osWaitForever) != osOK)
            continue;

        switch (s_fsmState) {

        case INV_STATE_INIT:
            if (msg.event == INV_EVT_INIT_DONE) {
                s_fsmState = INV_STATE_STANDBY;
                fsm_enter_standby();
            } else if (msg.event == INV_EVT_FAULT) {
                s_fsmState = INV_STATE_FAULT;
                fsm_enter_fault(msg.faultSource);
            }
            break;

        case INV_STATE_STANDBY:
            if (msg.event == INV_EVT_START) {
                s_fsmState = INV_STATE_ACTIVE;
                fsm_enter_active();
            } else if (msg.event == INV_EVT_FAULT) {
                s_fsmState = INV_STATE_FAULT;
                fsm_enter_fault(msg.faultSource);
            }
            break;

        case INV_STATE_ACTIVE:
            if (msg.event == INV_EVT_STOP) {
                s_fsmState = INV_STATE_STANDBY;
                fsm_enter_standby();
            } else if (msg.event == INV_EVT_FAULT) {
                s_fsmState = INV_STATE_FAULT;
                fsm_enter_fault(msg.faultSource);
            }
            break;

        case INV_STATE_FAULT:
            if (msg.event == INV_EVT_RESET) {
                s_fsmState = INV_STATE_RESET;
                fsm_enter_reset();
            }
            break;

        case INV_STATE_RESET:
            if (msg.event == INV_EVT_RESET_DONE) {
                s_fsmState = INV_STATE_STANDBY;
                fsm_enter_standby();
            } else if (msg.event == INV_EVT_FAULT) {
                s_fsmState = INV_STATE_FAULT;
                fsm_enter_fault(msg.faultSource);
            }
            break;

        default:
            break;
        }
    }
}

/* ─────────────────────────────────────────────────────────────────────────── */

void initCoreTasks(void)
{
    s_fsmQueue = osMessageQueueNew(8, sizeof(InverterFsmMsg_t), NULL);

    usbDeviceTaskHandle  = osThreadNew(usbDeviceTask,  NULL, &usb_device_task_attributes);
    scrutinyTaskHandle   = osThreadNew(scrutinyTask,   NULL, &scrutinyTask_attributes);
    userCDCTaskHandle    = osThreadNew(userCDCTask,    NULL, &userCDC_task_attributes);
    inverterFsmTaskHandle = osThreadNew(inverterFsmTask, NULL, &inverterFsmTask_attributes);
}