#include "core_tasks.h"
#include "cmsis_os.h"
#include "main.h"
#include "tusb.h"
#include "scrutiny_integration.h"
#include <string.h>

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

osThreadId_t userCdcTaskHandle;
const osThreadAttr_t userCdcTask_attributes = {
    .name = "userCdcTask",
    .stack_size = 512 * 4,
    .priority = osPriorityNormal,
};

void userCdcTask(void *param)
{
    while (1) {
        uint8_t rx_buf[64];
        uint32_t count = tud_cdc_n_read(USER_CDC, rx_buf, sizeof(rx_buf));
        if (count > 0) {
            const char *response;
            if (rx_buf[0] == '0') {
                HAL_GPIO_WritePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin, GPIO_PIN_RESET);
                response = "LED_ACTIVE OFF\r\n";
            } else if (rx_buf[0] == '1') {
                HAL_GPIO_WritePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin, GPIO_PIN_SET);
                response = "LED_ACTIVE ON\r\n";
            } else {
                response = "ERR: send '0' or '1'\r\n";
            }
            tud_cdc_n_write(USER_CDC, (const uint8_t *)response, strlen(response));
            tud_cdc_n_write_flush(USER_CDC);
        }
        osThreadYield();
    }
}

void initCoreTasks(void)
{
    usbDeviceTaskHandle = osThreadNew(usbDeviceTask, NULL, &usb_device_task_attributes);
    scrutinyTaskHandle = osThreadNew(scrutinyTask, NULL, &scrutinyTask_attributes);
    userCdcTaskHandle = osThreadNew(userCdcTask, NULL, &userCdcTask_attributes);
}