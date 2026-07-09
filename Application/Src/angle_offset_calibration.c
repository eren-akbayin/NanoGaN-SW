#include "angle_offset_calibration.h"
#include "cmsis_os.h"
#include "core_tasks.h"
#include "parameters.h"

#define ANGLE_OFFSET_CALIBRATION_WAKE_FLAG 0x01U
#define ANGLE_OFFSET_CALIBRATION_RETRY_MS  10U
#define ANGLE_SAMPLES 100


uint32_t uAngleSum;

static osThreadId_t s_taskHandle;
static const osThreadAttr_t s_taskAttributes = {
    .name       = "angleOffsetCalibration",
    .stack_size = 256 * 4,
    .priority   = osPriorityNormal,
};

static volatile uint32_t s_timeoutMs;

static void angleOffsetCalibration(void *argument)
{
    (void)argument;

    osThreadFlagsWait(ANGLE_OFFSET_CALIBRATION_WAKE_FLAG, osFlagsWaitAny, osWaitForever);

    uint32_t elapsedMs = 0;
        while ((inverter_fsm_get_state() != INV_STATE_ACTIVE) && (elapsedMs < s_timeoutMs)) {
            inverter_fsm_post_event(INV_EVT_START);
            osDelay(ANGLE_OFFSET_CALIBRATION_RETRY_MS);
            elapsedMs += ANGLE_OFFSET_CALIBRATION_RETRY_MS;}
    
            
    if (inverter_fsm_get_state() == INV_STATE_ACTIVE) {
        
        gParameters.fDutyD = 0.2f; // Apply a small D-axis duty cycle to generate a known current for calibration
        gParameters.uAngleSelection = 0; // Use manual angle for calibration
        gParameters.uIncrementManual = 0; // No increment during calibration
        gParameters.uAngleManual = 0; // Start from zero angle

        uAngleSum = 0;

        for (int i = 0; i < ANGLE_SAMPLES; i++) {
            
            uAngleSum += gParameters.uAngleMech; // Capture the mechanical angle

            if (inverter_fsm_get_state() != INV_STATE_ACTIVE) {
                break; // Exit if the inverter is no longer active
                uAngleSum = 0; // Reset the sum if the inverter is not active
            }

            osDelay(1);
        }

        gParameters.uAngleOffset += (uint16_t)(uAngleSum / ANGLE_SAMPLES); // Average the captured angles
 
    }

    inverter_fsm_post_event(INV_EVT_STOP); // Stop the inverter after calibration
    
    gParameters.fDutyD = 0.0f; // Reset D-axis duty cycle

    osThreadExit();
}

void AngleOffsetCalibration_Init(void)
{
    s_taskHandle = osThreadNew(angleOffsetCalibration, NULL, &s_taskAttributes);
}

void AngleOffsetCalibration_Wake(uint32_t timeout_ms)
{
    s_timeoutMs = timeout_ms;
    osThreadFlagsSet(s_taskHandle, ANGLE_OFFSET_CALIBRATION_WAKE_FLAG);
}
