#include "mc_math.h"

uint16_t Angle_RawToMechanical(uint16_t uAngleRaw)
{
    return (uint16_t)(-(uint16_t)((uAngleRaw & 0x3FFF) << 2));
}

uint16_t Angle_MechanicalToElectrical(uint16_t uAngleMech, uint8_t uPolePairs, uint16_t uAngleOffset)
{
    return (uint16_t)((uint32_t)uAngleMech * uPolePairs) + uAngleOffset;
}

void Clarke_Forward(const float fPhase[3], float *pfAlpha, float *pfBeta)
{
    *pfAlpha = (2.0f / 3.0f) * (fPhase[0] - 0.5f * fPhase[1] - 0.5f * fPhase[2]);
    *pfBeta  = (1.0f / 1.732050808f) * (fPhase[1] - fPhase[2]);
}

void Clarke_Inverse(float fAlpha, float fBeta, float fPhase[3])
{
    fPhase[0] = fAlpha;
    fPhase[1] = -fAlpha * 0.5f + fBeta * 0.866025f;
    fPhase[2] = -fAlpha * 0.5f - fBeta * 0.866025f;
}

void Park_Forward(float fAlpha, float fBeta, float fSinTheta, float fCosTheta, float *pfD, float *pfQ)
{
    *pfD =  fAlpha * fCosTheta + fBeta * fSinTheta;
    *pfQ = -fAlpha * fSinTheta + fBeta * fCosTheta;
}

void Park_Inverse(float fD, float fQ, float fSinTheta, float fCosTheta, float *pfAlpha, float *pfBeta)
{
    *pfAlpha = fD * fCosTheta - fQ * fSinTheta;
    *pfBeta  = fD * fSinTheta + fQ * fCosTheta;
}
