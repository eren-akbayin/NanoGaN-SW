#pragma once

#include <stdint.h>
#include "measurement.h"  /* measurementType_t, faultType_t */

/* ── Inverter FSM ─────────────────────────────────────────────────────────── */

typedef enum {
    INV_STATE_INIT = 0,
    INV_STATE_STANDBY,
    INV_STATE_ACTIVE,
    INV_STATE_RESET,
    INV_STATE_FAULT,
} InverterState_t;

typedef enum {
    INV_EVT_INIT_DONE = 0, /* init/calibration complete – go to STANDBY */
    INV_EVT_START,         /* request transition to ACTIVE  */
    INV_EVT_STOP,          /* request transition to STANDBY */
    INV_EVT_FAULT,         /* fault detected – go to FAULT  */
    INV_EVT_RESET,         /* initiate reset sequence        */
    INV_EVT_RESET_DONE,    /* reset complete – back to STANDBY */
} InverterEvent_t;

typedef struct {
    InverterEvent_t   event;
    measurementType_t faultSource;  /* valid only when event == INV_EVT_FAULT */
} InverterFsmMsg_t;

/* General-purpose event (non-fault transitions) */
void inverter_fsm_post_event(InverterEvent_t evt);

/* Fault trigger — ISR-safe; carries the fault source so FSM can call getShutdownInfo() */
void inverter_fsm_post_fault(measurementType_t source);

/* Query current state (read-only snapshot) */
InverterState_t inverter_fsm_get_state(void);

/* ─────────────────────────────────────────────────────────────────────────── */

extern void initCoreTasks(void);