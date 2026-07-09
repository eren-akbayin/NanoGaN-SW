#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Raw angle sensor reading -> mechanical angle */
uint16_t Angle_RawToMechanical(uint16_t uAngleRaw, uint16_t uAngleOffset);

/* Mechanical angle -> electrical angle (applies pole-pair scaling and offset) */
uint16_t Angle_MechanicalToElectrical(uint16_t uAngleMech, uint8_t uPolePairs);

/* Clarke transform: 3-phase quantities -> stationary alpha/beta frame */
void Clarke_Forward(const float fPhase[3], float *pfAlpha, float *pfBeta);

/* Inverse Clarke transform: stationary alpha/beta frame -> 3-phase quantities (range -1..1) */
void Clarke_Inverse(float fAlpha, float fBeta, float fPhase[3]);

/* Park transform: stationary alpha/beta frame -> rotating d/q frame */
void Park_Forward(float fAlpha, float fBeta, float fSinTheta, float fCosTheta, float *pfD, float *pfQ);

/* Inverse Park transform: rotating d/q frame -> stationary alpha/beta frame */
void Park_Inverse(float fD, float fQ, float fSinTheta, float fCosTheta, float *pfAlpha, float *pfBeta);

#ifdef __cplusplus
}
#endif
