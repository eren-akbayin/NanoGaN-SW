#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the task. It blocks immediately and waits to be woken up. */
void AngleOffsetCalibration_Init(void);

/* Wakes the task: it repeatedly posts INV_EVT_START to the inverter FSM,
   retrying every 10ms, until the FSM reaches INV_STATE_ACTIVE or timeout_ms
   elapses. Can be called again once the previous run has finished. */
void AngleOffsetCalibration_Wake(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
