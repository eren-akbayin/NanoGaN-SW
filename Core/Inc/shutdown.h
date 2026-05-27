/*
 * shutdown.h
 *
 *  Created on: Apr 18, 2026
 *      Author: efakb
 */

#pragma once

#include "main.h"

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
	uint32_t dmaIndexCurrent;
	uint32_t dmaIndexPhaseVoltage;
	uint32_t dmaIndexDCVoltage;
	uint32_t dmaIndexAngle;
	uint32_t faultIndex;
} shutdownInfoTypeDef_t;

extern ADC_AnalogWDGConfTypeDef AnalogWDGConfig_Currents;
extern ADC_AnalogWDGConfTypeDef AnalogWDGConfig_VoltageDc;

extern void calibrateSensorsSetShutdowns(float i_max, float u_min, float u_max);
extern void gateDriveShutdown(void);
extern void getShutdownInfo(measurementType_t measurementType);
