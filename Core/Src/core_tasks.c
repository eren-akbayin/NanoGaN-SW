#include "core_tasks.h"
#include "class/cdc/cdc_device.h"
#include "main.h"


osThreadId_t usbDeviceTaskHandle;
const osThreadAttr_t usb_device_task_attributes = {
    .name = "usbDeviceTask",
    .stack_size = 1024 * 4,
    .priority = osPriorityHigh,
};

void usbDeviceTask(void *param)
{
    tusb_init();

    while (1) {
        tud_task();
    }
}

osThreadId_t cdcTaskHandle;
const osThreadAttr_t cdcTask_attributes = {
    .name = "cdcTask",
    .stack_size = 512 * 4,
    .priority = osPriorityNormal,
};

void cdcTask(void *param)
{
    osDelay(500); // wait for USB stack to be ready
    scrutiny_integration_init();

    while (1) {
        uint8_t buf[64];
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        if (count > 0) {
            // Toggle orange LED to confirm we're receiving bytes
            HAL_GPIO_TogglePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin);
            scrutiny_receive_data(buf, (uint16_t)count);
        }

        // Toggle blue LED every loop to confirm task is running
        HAL_GPIO_TogglePin(LED_ACTIVE_GPIO_Port, LED_ACTIVE_Pin);

        scrutiny_process_and_send();
        osThreadYield();
    }
}

void initCoreTasks(void)
{
    usbDeviceTaskHandle = osThreadNew(usbDeviceTask, NULL, &usb_device_task_attributes);
    cdcTaskHandle = osThreadNew(cdcTask, NULL, &cdcTask_attributes);
}