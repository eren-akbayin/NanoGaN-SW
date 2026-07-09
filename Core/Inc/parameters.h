#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POLE_PAIR 4
#define ANGLE_OFFSET 0

typedef enum {
    PARAM_ANGLE_OFFSET = 0,     /* uint16_t - electrical angle offset [encoder counts] */
    PARAM_POLE_PAIRS,           /* uint8_t  - motor pole pairs                         */
    PARAM_STATOR_RESISTANCE,    /* float    - stator phase resistance Rs [Ohm]         */
    PARAM_STATOR_INDUCTANCE_D,  /* float    - d-axis stator inductance Ld [H]          */
    PARAM_STATOR_INDUCTANCE_Q,  /* float    - q-axis stator inductance Lq [H]          */
    PARAM_FLUX_LINKAGE,         /* float    - permanent magnet flux linkage [Wb]       */
    PARAM_MAX_PHASE_CURRENT,    /* float    - overcurrent protection limit [A]         */
    PARAM_MIN_DC_VOLTAGE,       /* float    - undervoltage protection limit [V]        */
    PARAM_MAX_DC_VOLTAGE,       /* float    - overvoltage protection limit [V]         */
    PARAM_MAX_SPEED,            /* float    - overspeed protection limit [rpm]         */
    PARAM_MAX_TEMPERATURE,      /* float    - overtemperature protection limit [degC]  */
    PARAM_DUTY_D,               /* float    - open-loop d-axis duty command            */
    PARAM_DUTY_Q,               /* float    - open-loop q-axis duty command            */
    PARAM_ANGLE_MANUAL,         /* uint16_t - manual electrical angle [encoder counts] */
    PARAM_ANGLE_SELECTION,      /* uint8_t  - 0 = manual angle, 1 = sensor-derived     */
    PARAM_INCREMENT_MANUAL,     /* uint16_t - per-cycle increment of the manual angle  */
    PARAM_ANGLE_MECH,           /* uint16_t - mechanical angle [encoder counts]        */
    PARAM_ANGLE_EL,             /* uint16_t - electrical angle [encoder counts]        */
    PARAM_ANGLE_RAW,            /* uint16_t - raw angle sensor reading (SPI/DMA)       */
    PARAM_COUNT
} ParamId_t;

typedef struct {
    uint16_t uAngleOffset;
    uint8_t  uPolePairs;
    float    fStatorResistance;
    float    fStatorInductanceD;
    float    fStatorInductanceQ;
    float    fFluxLinkage;
    float    fMaxPhaseCurrent;
    float    fMinDcVoltage;
    float    fMaxDcVoltage;
    float    fMaxSpeed;
    float    fMaxTemperature;
    float    fDutyD;
    float    fDutyQ;
    uint16_t uAngleManual;
    uint8_t  uAngleSelection;
    uint16_t uIncrementManual;
    uint16_t uAngleMech;
    uint16_t uAngleEl;
    volatile uint16_t uAngleRaw; /* SPI2 RX-DMA target; gParameters must stay in .dma_data (RAM_D3) */
} Parameters_t;

extern Parameters_t gParameters;

/* Invoked from Parameters_Set() / Parameters_Poll() whenever a parameter's value actually changes */
typedef void (*ParamChangedCallback_t)(ParamId_t paramId, float fOldValue, float fNewValue);

/* Loads default values (from the existing hardware constants) and clears all callbacks */
void Parameters_Init(void);

/* Generic accessors, valid for every parameter (integer-backed ones are exact in float) */
float Parameters_Get(ParamId_t paramId);
void Parameters_Set(ParamId_t paramId, float fValue);

/* Registers the callback fired when paramId's value changes. One callback per parameter;
   registering again replaces the previous one. Pass NULL to unregister. */
void Parameters_RegisterCallback(ParamId_t paramId, ParamChangedCallback_t callback);

/* Detects out-of-band changes (e.g. a debugger/Scrutiny writing gParameters directly instead
   of going through Parameters_Set()) and fires the matching callbacks. Call periodically from
   a task context. */
void Parameters_Poll(void);

#ifdef __cplusplus
}
#endif
