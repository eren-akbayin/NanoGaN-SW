#include "user_task.h"
#include "cmsis_os.h"
#include "angle_offset_calibration.h"
#include "supermario_task.h"

#define MAIN_APP_TASK_WAKE_FLAG 0x01U
#define FSM_ACTIVE_TIMEOUT_MS   5000U

uint8_t uSuperMario = 0;
uint8_t uSongSelect = 0;


static osThreadId_t s_taskHandle;
static const osThreadAttr_t s_taskAttributes = {
    .name       = "mainAppTask",
    .stack_size = 256 * 4,
    .priority   = osPriorityNormal,
};

static void mainAppTask(void *argument)
{
    (void)argument;

    osThreadFlagsWait(MAIN_APP_TASK_WAKE_FLAG, osFlagsWaitAny, osWaitForever);

    AngleOffsetCalibration_Wake(FSM_ACTIVE_TIMEOUT_MS);

    for (;;) {
        if (uSuperMario == 1) {
            uSuperMario = 0;
            SuperMarioTask_Wake();
        }
        osDelay(10);
    }
}

void MainAppTask_Init(void)
{
    AngleOffsetCalibration_Init();
    SuperMarioTask_Init();
    s_taskHandle = osThreadNew(mainAppTask, NULL, &s_taskAttributes);
}

void MainAppTask_Wake(void)
{
    osThreadFlagsSet(s_taskHandle, MAIN_APP_TASK_WAKE_FLAG);
}
