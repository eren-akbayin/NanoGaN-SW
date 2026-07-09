#include "parameters.h"
#include "measurement.h"
#include <string.h>

Parameters_t gParameters __attribute__((section(".dma_data")));

static float s_lastValues[PARAM_COUNT];
static ParamChangedCallback_t s_callbacks[PARAM_COUNT];

float Parameters_Get(ParamId_t paramId)
{
    switch (paramId)
    {
    case PARAM_ANGLE_OFFSET:        return (float)gParameters.uAngleOffset;
    case PARAM_POLE_PAIRS:          return (float)gParameters.uPolePairs;
    case PARAM_STATOR_RESISTANCE:   return gParameters.fStatorResistance;
    case PARAM_STATOR_INDUCTANCE_D: return gParameters.fStatorInductanceD;
    case PARAM_STATOR_INDUCTANCE_Q: return gParameters.fStatorInductanceQ;
    case PARAM_FLUX_LINKAGE:        return gParameters.fFluxLinkage;
    case PARAM_MAX_PHASE_CURRENT:   return gParameters.fMaxPhaseCurrent;
    case PARAM_MIN_DC_VOLTAGE:      return gParameters.fMinDcVoltage;
    case PARAM_MAX_DC_VOLTAGE:      return gParameters.fMaxDcVoltage;
    case PARAM_MAX_SPEED:           return gParameters.fMaxSpeed;
    case PARAM_MAX_TEMPERATURE:     return gParameters.fMaxTemperature;
    case PARAM_DUTY_D:              return gParameters.fDutyD;
    case PARAM_DUTY_Q:              return gParameters.fDutyQ;
    case PARAM_ANGLE_MANUAL:        return (float)gParameters.uAngleManual;
    case PARAM_ANGLE_SELECTION:     return (float)gParameters.uAngleSelection;
    case PARAM_INCREMENT_MANUAL:    return (float)gParameters.uIncrementManual;
    case PARAM_ANGLE_MECH:          return (float)gParameters.uAngleMech;
    case PARAM_ANGLE_EL:            return (float)gParameters.uAngleEl;
    case PARAM_ANGLE_RAW:           return (float)gParameters.uAngleRaw;
    default:                        return 0.0f;
    }
}

static void Parameters_Write(ParamId_t paramId, float fValue)
{
    switch (paramId)
    {
    case PARAM_ANGLE_OFFSET:        gParameters.uAngleOffset       = (uint16_t)fValue; break;
    case PARAM_POLE_PAIRS:          gParameters.uPolePairs         = (uint8_t)fValue;  break;
    case PARAM_STATOR_RESISTANCE:   gParameters.fStatorResistance  = fValue;           break;
    case PARAM_STATOR_INDUCTANCE_D: gParameters.fStatorInductanceD = fValue;           break;
    case PARAM_STATOR_INDUCTANCE_Q: gParameters.fStatorInductanceQ = fValue;           break;
    case PARAM_FLUX_LINKAGE:        gParameters.fFluxLinkage       = fValue;           break;
    case PARAM_MAX_PHASE_CURRENT:   gParameters.fMaxPhaseCurrent   = fValue;           break;
    case PARAM_MIN_DC_VOLTAGE:      gParameters.fMinDcVoltage      = fValue;           break;
    case PARAM_MAX_DC_VOLTAGE:      gParameters.fMaxDcVoltage      = fValue;           break;
    case PARAM_MAX_SPEED:           gParameters.fMaxSpeed          = fValue;           break;
    case PARAM_MAX_TEMPERATURE:     gParameters.fMaxTemperature    = fValue;           break;
    case PARAM_DUTY_D:              gParameters.fDutyD             = fValue;           break;
    case PARAM_DUTY_Q:              gParameters.fDutyQ             = fValue;           break;
    case PARAM_ANGLE_MANUAL:        gParameters.uAngleManual       = (uint16_t)fValue; break;
    case PARAM_ANGLE_SELECTION:     gParameters.uAngleSelection    = (uint8_t)fValue;  break;
    case PARAM_INCREMENT_MANUAL:    gParameters.uIncrementManual   = (uint16_t)fValue; break;
    case PARAM_ANGLE_MECH:          gParameters.uAngleMech         = (uint16_t)fValue; break;
    case PARAM_ANGLE_EL:            gParameters.uAngleEl           = (uint16_t)fValue; break;
    case PARAM_ANGLE_RAW:           gParameters.uAngleRaw          = (uint16_t)fValue; break;
    default: break;
    }
}

static void Parameters_Notify(ParamId_t paramId, float fOldValue, float fNewValue)
{
    if (fNewValue != fOldValue && s_callbacks[paramId] != NULL)
    {
        s_callbacks[paramId](paramId, fOldValue, fNewValue);
    }
}

void Parameters_Set(ParamId_t paramId, float fValue)
{
    if (paramId >= PARAM_COUNT) return;

    float fOldValue = s_lastValues[paramId];
    Parameters_Write(paramId, fValue);
    float fNewValue = Parameters_Get(paramId); /* re-read: integer-backed params truncate fValue */
    s_lastValues[paramId] = fNewValue;

    Parameters_Notify(paramId, fOldValue, fNewValue);
}

void Parameters_RegisterCallback(ParamId_t paramId, ParamChangedCallback_t callback)
{
    if (paramId >= PARAM_COUNT) return;
    s_callbacks[paramId] = callback;
}

void Parameters_Poll(void)
{
    for (int i = 0; i < PARAM_COUNT; i++)
    {
        ParamId_t paramId = (ParamId_t)i;
        float fNewValue = Parameters_Get(paramId);
        float fOldValue = s_lastValues[paramId];

        if (fNewValue != fOldValue)
        {
            s_lastValues[paramId] = fNewValue;
            Parameters_Notify(paramId, fOldValue, fNewValue);
        }
    }
}

void Parameters_Init(void)
{
    memset(&gParameters, 0, sizeof(gParameters));
    memset(s_callbacks, 0, sizeof(s_callbacks));

    /* Defaults mirror the constants previously hardcoded at the call sites */
    gParameters.uAngleOffset       = ANGLE_OFFSET;
    gParameters.uPolePairs         = POLE_PAIR;
    gParameters.fStatorResistance  = 543.0f;
    gParameters.fStatorInductanceD = 182.0f;
    gParameters.fStatorInductanceQ = 190.0f;
    gParameters.fFluxLinkage       = 0.0f;
    gParameters.fMaxPhaseCurrent   = 5.0f;
    gParameters.fMinDcVoltage      = 11.0f;
    gParameters.fMaxDcVoltage      = 15.0f;
    gParameters.fMaxSpeed          = 0.0f;
    gParameters.fMaxTemperature    = 0.0f;
    gParameters.fDutyD             = 0.0f;
    gParameters.fDutyQ             = 0.0f;
    gParameters.uAngleManual       = 0;
    gParameters.uAngleSelection    = 0;
    gParameters.uIncrementManual   = 0;
    gParameters.uAngleMech         = 0;
    gParameters.uAngleEl           = 0;
    gParameters.uAngleRaw          = 0;

    for (int i = 0; i < PARAM_COUNT; i++)
    {
        s_lastValues[i] = Parameters_Get((ParamId_t)i);
    }
}
