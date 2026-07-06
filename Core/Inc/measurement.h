#pragma once

#include "main.h"
#include "tim.h"

//Measurement conversion constants

#define VOLTS_PER_BIT 12.0f/790.0f
#define AMPERES_PER_BIT 80.0f/4096.0f
#define BITS_PER_AMPERE 4096.0f/80.0f
#define BITS_PER_VOLT 790.0f/12.0f
#define DEGREES_PER_BIT 360.0f/16384.0f



#define POLE_PAIR 4
#define ANGLE_OFFSET 56005

// DMA Pointers

#define PHASE_VOLTAGE_DMA_NDTR 	0x40020014 //DMA1 Stream 0 NDTR
#define PHASE_CURRENT_DMA_NDTR 	0x4002002c //DMA1 Stream 1 NDTR
#define DC_VOLTAGE_DMA_NDTR 	0x40020044 //DMA1 Stream 2 NDTR
#define ANGLE_DMA_NDTR		    0x40020414 //DMA2 Stream 0 NDTR

// Typedef for measurement and shutdown

// Measurement
typedef struct {

    // Current offsets
    volatile uint16_t uCurrOffsetU;
    volatile uint16_t uCurrOffsetV;
    volatile uint16_t uCurrOffsetW;

    // Voltage & sensor measurements
    uint32_t uDcLinkVoltage[10];
    uint32_t uPhaseSens[3*10];
    uint32_t uCurrSens[3*10];

	uint16_t uAngleOffset;
    uint16_t uAngleEl;

    uint8_t uPolePair;

} inverterMeasurementsTypeDef_t;

// Shutdown stuff

typedef enum
{
	OC = 0,		    // Open Circuit
	ASC_LOW = 1,	// Active Short Circuit Low Side
	ASC_HIGH = 2	// Active Short Circuit High Side. Careful due to bootstrap it will persist for a limited time!!!
}shutdownType_t;

typedef enum
{
	NONE = 0, VOLTAGE = 1, CURRENT = 2, TEMPERATURE = 3, ANGLE = 4, SPEED = 5
} measurementType_t;

typedef enum
{
	NO_FAULT = 0,
	UNDER_VOLTAGE_FAULT = 1,
	OVER_VOLTAGE_FAULT = 2,
	OVER_CURRENT_FAULT_PHASE_U = 3,
	OVER_CURRENT_FAULT_PHASE_V = 4,
	OVER_CURRENT_FAULT_PHASE_W = 5,
	OVER_TEMPERATURE_FAULT = 6,
	OVER_SPEED_FAULT = 7
} faultType_t;

typedef struct
{
	/* ── Threshold Limits ──────────────────────────────────────── */
	uint16_t upperThresholdRaw; /* Configured upper limit (raw)     */
	uint16_t lowerThresholdRaw; /* Configured lower limit (raw)     */
	float upperThreshold; /* Configured upper limit [V or A]        */
	float lowerThreshold; /* Configured lower limit [V or A]        */
} thresholdTypeDef_t;

typedef struct
{
	uint16_t measuredRaw;
	float measured;
	measurementType_t measurementType;
	thresholdTypeDef_t thresholds;
	faultType_t faultType;
	uint32_t dmaIndexPhaseCurrents;
	uint32_t dmaIndexPhaseVoltages;
	uint32_t dmaIndexDCVoltage;
	uint32_t dmaIndexAngle;
	uint32_t faultIndex;
} shutdownInfoTypeDef_t;

//Export scructs

extern inverterMeasurementsTypeDef_t gInverterMeasurements;
extern shutdownType_t gShutdownType;

//Export functions
extern void stopMeasurements(void);
extern void startMeasurements(void);
extern void calibrateSensorsSetShutdowns(float i_max, float u_min, float u_max);
extern void startGateDrive(void);
extern void stopGateDrive(void);
extern void shutdownGateDrive(void);
extern void getShutdownInfo(measurementType_t measurementType);
extern void clearShutdownInfo();

//Export floats for d and q currents
extern float fCurrD;
extern float fCurrQ;