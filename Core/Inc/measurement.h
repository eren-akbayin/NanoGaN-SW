#pragma once

#include "main.h"

#define MEASUREMENT_SIZE 10

typedef struct {
    //Buffer length
    uint32_t bufferSize;

    //Indexes
    uint32_t dmaIndexCurrent;
    uint32_t dmaIndexPhaseVoltage;
    uint32_t dmaIndexDCVoltage;
    uint32_t dmaIndexAngle;
    uint32_t faultIndex;

    //Time Step
    uint32_t uTimeStepUs;

    //Conversion constants
    float fVoltPerBit;
    float fAmperePerbit;

    // Current offsets
    volatile uint16_t uCurrOffsetU;
    volatile uint16_t uCurrOffsetV;
    volatile uint16_t uCurrOffsetW;

    // Voltage & sensor measurements
    uint32_t uDcLinkVoltage[MEASUREMENT_SIZE];
    uint32_t uPhaseSens[3*MEASUREMENT_SIZE];
    uint32_t uCurrSens[3*MEASUREMENT_SIZE];

    uint16_t uMechPosition[MEASUREMENT_SIZE / 2];

    uint16_t uAngleoffset;
    uint8_t uPolePair;

    uint16_t indexDTC;
    uint16_t uDTC[MEASUREMENT_SIZE/10][3];

} inverterMeasurementsTypeDef_t;

extern inverterMeasurementsTypeDef_t gInverterMeasurements;