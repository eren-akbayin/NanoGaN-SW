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
	NO_SHUTDOWN = 0,
	UNDER_VOLTAGE_SHUTDOWN = 1,
	OVER_VOLTAGE_SHUTDOWN = 2,
	OVER_CURRENT_SHUTDOWN_PHASE_U = 3,
	OVER_CURRENT_SHUTDOWN_PHASE_V = 4,
	OVER_CURRENT_SHUTDOWN_PHASE_W = 5,
	OVER_TEMPERATURE_SHUTDOWN = 6,
	OVER_SPEED_SHUTDOWN = 7
} shutdownType_t;

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
	shutdownType_t shutdownType;
	uint32_t dmaIndex;
	uint32_t faultIndex;
} shutdownInfoTypeDef_t;

extern ADC_AnalogWDGConfTypeDef AnalogWDGConfig_Currents;
extern ADC_AnalogWDGConfTypeDef AnalogWDGConfig_VoltageDc;

extern void calibrateSensorsSetShutdowns(float i_max, float u_min, float u_max);
extern void gateDriveShutdown(void);
extern void getShutdownInfo(measurementType_t measurementType, uint32_t dmaPointer);
