/*
 * shutdown.c
 *
 *  Created on: Apr 18, 2026
 *      Author: efakb
 */
#include "shutdown.h"
#include "app_threadx.h"
#include "adc.h"
#include "tim.h"

// Function prototypes (static)

static int32_t calibrateOffset(uint32_t *pData, uint32_t len, uint8_t offset);

ADC_AnalogWDGConfTypeDef AnalogWDGConfig_Currents = {0};
ADC_AnalogWDGConfTypeDef AnalogWDGConfig_VoltageDc = {0};

shutdownInfoTypeDef_t shutdownInfo;

static int32_t calibrateOffset(uint32_t *pData, uint32_t len, uint8_t offset)
{

	int64_t sum = 0;

	uint32_t count = 0;

	for (uint32_t i = offset; i < len; i = i + 3)
			{
		sum += (int32_t)pData[i];
		count++;
	}

	return (int32_t)(sum / count);
}

void calibrateSensorsSetShutdowns(float i_max, float u_min, float u_max)
{

	HAL_ADC_Stop_DMA(&hadc1);
	HAL_ADC_Stop_DMA(&hadc2);
	HAL_ADC_Stop_DMA(&hadc3);

	HAL_TIM_Base_Stop(&htim2);

	HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);

	HAL_ADC_Start_DMA(&hadc2, gInverterMeasurements.uCurrSens, 3 * MEASUREMENT_SIZE);

	HAL_TIM_Base_Start(&htim2);

	tx_thread_sleep(100);

	HAL_TIM_Base_Stop(&htim2);

	HAL_ADC_Stop_DMA(&hadc2);

	gInverterMeasurements.uCurrOffsetU = calibrateOffset(gInverterMeasurements.uCurrSens, 3 * MEASUREMENT_SIZE, 0);
	gInverterMeasurements.uCurrOffsetV = calibrateOffset(gInverterMeasurements.uCurrSens, 3 * MEASUREMENT_SIZE, 1);
	gInverterMeasurements.uCurrOffsetW = calibrateOffset(gInverterMeasurements.uCurrSens, 3 * MEASUREMENT_SIZE, 2);

	uint32_t uAwdHigh = MIN3(gInverterMeasurements.uCurrOffsetU, gInverterMeasurements.uCurrOffsetV, gInverterMeasurements.uCurrOffsetW)
			+ (uint32_t)(BITS_PER_AMPERE * i_max);
	uint32_t uAwdLow = MAX3(gInverterMeasurements.uCurrOffsetU, gInverterMeasurements.uCurrOffsetV, gInverterMeasurements.uCurrOffsetW)
			- (uint32_t)(BITS_PER_AMPERE * i_max);

	AnalogWDGConfig_Currents.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
	AnalogWDGConfig_Currents.WatchdogMode = ADC_ANALOGWATCHDOG_ALL_REG;
	AnalogWDGConfig_Currents.ITMode = ENABLE;
	AnalogWDGConfig_Currents.HighThreshold = uAwdHigh;
	AnalogWDGConfig_Currents.LowThreshold = uAwdLow;

	if (HAL_ADC_AnalogWDGConfig(&hadc2, &AnalogWDGConfig_Currents)
			!= HAL_OK)
			{
		Error_Handler();
	}

	AnalogWDGConfig_VoltageDc.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
	AnalogWDGConfig_VoltageDc.WatchdogMode = ADC_ANALOGWATCHDOG_ALL_REG;
	AnalogWDGConfig_VoltageDc.ITMode = ENABLE;
	AnalogWDGConfig_VoltageDc.HighThreshold = (uint32_t)(u_max * BITS_PER_VOLT);
	AnalogWDGConfig_VoltageDc.LowThreshold = (uint32_t)(u_min * BITS_PER_VOLT);
	AnalogWDGConfig_VoltageDc.FilteringConfig = ADC3_AWD_FILTERING_NONE;
	if (HAL_ADC_AnalogWDGConfig(&hadc3, &AnalogWDGConfig_VoltageDc)
			!= HAL_OK)
			{
		Error_Handler();
	}

	HAL_ADC_Start_DMA(&hadc1, gInverterMeasurements.uPhaseSens, 3 * MEASUREMENT_SIZE);

	HAL_ADC_Start_DMA(&hadc2, gInverterMeasurements.uCurrSens, 3 * MEASUREMENT_SIZE);

	HAL_ADC_Start_DMA(&hadc3, gInverterMeasurements.uDcLinkVoltage, 3 * MEASUREMENT_SIZE);

	HAL_TIM_Base_Start(&htim2);

}

void gateDriveShutdown(void)
{

	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
	TIM1->CCR3 = ARR_VAL;
	TIM1->CCR2 = ARR_VAL;
	TIM1->CCR1 = ARR_VAL;
}

void getShutdownInfo(measurementType_t measurementType, uint32_t dmaIndex)
{
	memset(&shutdownInfo, 0, sizeof(shutdownInfo));
	shutdownInfo.measurementType = measurementType;
	shutdownInfo.dmaIndex = dmaIndex;
	uint16_t i = dmaIndex + 1;

	if (measurementType == VOLTAGE)
			{
		shutdownInfo.thresholds.lowerThresholdRaw = AnalogWDGConfig_VoltageDc.LowThreshold;
		shutdownInfo.thresholds.upperThresholdRaw = AnalogWDGConfig_VoltageDc.HighThreshold;
		do
		{
			if (gInverterMeasurements.uDcLinkVoltage[i] < shutdownInfo.thresholds.lowerThresholdRaw)
					{
				shutdownInfo.shutdownType = UNDER_VOLTAGE_SHUTDOWN;
				break;
			}
			else if (gInverterMeasurements.uDcLinkVoltage[i] > shutdownInfo.thresholds.upperThresholdRaw)
					{
				shutdownInfo.shutdownType = OVER_VOLTAGE_SHUTDOWN;
				break;
			}

			i++;

			if (i == 3 * MEASUREMENT_SIZE)
				i = 0;

		} while (i != dmaIndex);

		shutdownInfo.faultIndex = i;

		shutdownInfo.measuredRaw = gInverterMeasurements.uDcLinkVoltage[shutdownInfo.faultIndex];
		shutdownInfo.measured = (float)(shutdownInfo.measuredRaw) * VOLTS_PER_BIT;
		shutdownInfo.thresholds.lowerThreshold = (float)(shutdownInfo.thresholds.lowerThresholdRaw) * VOLTS_PER_BIT;
		shutdownInfo.thresholds.upperThreshold = (float)(shutdownInfo.thresholds.upperThresholdRaw) * VOLTS_PER_BIT;
	}
	else if (measurementType == CURRENT)
			{
		shutdownInfo.thresholds.lowerThresholdRaw = AnalogWDGConfig_Currents.LowThreshold;
		shutdownInfo.thresholds.upperThresholdRaw = AnalogWDGConfig_Currents.HighThreshold;

		do
		{
			if (gInverterMeasurements.uCurrSens[i] < shutdownInfo.thresholds.lowerThresholdRaw
					|| gInverterMeasurements.uCurrSens[i] > shutdownInfo.thresholds.upperThresholdRaw)
				break;

			i++;

			if (i == 3 * MEASUREMENT_SIZE)
				i = 0;

		} while (i != dmaIndex);

		shutdownInfo.faultIndex = i;
		shutdownInfo.measuredRaw = gInverterMeasurements.uCurrSens[shutdownInfo.faultIndex];

		switch (shutdownInfo.faultIndex % 3)
		{
		case 0:
			shutdownInfo.shutdownType = OVER_CURRENT_SHUTDOWN_PHASE_U;
			shutdownInfo.measured = (float)((int32_t)gInverterMeasurements.uCurrSens[shutdownInfo.faultIndex]
					- (int32_t)gInverterMeasurements.uCurrOffsetU) * -AMPERES_PER_BIT;
			shutdownInfo.thresholds.lowerThreshold = -(float)(AnalogWDGConfig_Currents.HighThreshold
					- gInverterMeasurements.uCurrOffsetV) * AMPERES_PER_BIT;
			shutdownInfo.thresholds.upperThreshold = (float)(gInverterMeasurements.uCurrOffsetV
					- AnalogWDGConfig_Currents.LowThreshold) * AMPERES_PER_BIT;
			break;
		case 1:
			shutdownInfo.shutdownType =
					OVER_CURRENT_SHUTDOWN_PHASE_V;
			shutdownInfo.measured = (float)((int32_t)gInverterMeasurements.uCurrSens[shutdownInfo.faultIndex]
					- (int32_t)gInverterMeasurements.uCurrOffsetV) * AMPERES_PER_BIT;
			shutdownInfo.thresholds.lowerThreshold = -(float)(gInverterMeasurements.uCurrOffsetV - AnalogWDGConfig_Currents.LowThreshold)
					* AMPERES_PER_BIT;
			shutdownInfo.thresholds.upperThreshold = (float)(AnalogWDGConfig_Currents.HighThreshold - gInverterMeasurements.uCurrOffsetV)
					* AMPERES_PER_BIT;
			break;
		case 2:
			shutdownInfo.shutdownType = OVER_CURRENT_SHUTDOWN_PHASE_W;
			shutdownInfo.measured = (float)((int32_t)gInverterMeasurements.uCurrSens[shutdownInfo.faultIndex]
					- (int32_t)gInverterMeasurements.uCurrOffsetW) * AMPERES_PER_BIT;
			shutdownInfo.thresholds.lowerThreshold = -(float)(gInverterMeasurements.uCurrOffsetW - AnalogWDGConfig_Currents.LowThreshold)
					* AMPERES_PER_BIT;
			shutdownInfo.thresholds.upperThreshold = (float)(AnalogWDGConfig_Currents.HighThreshold - gInverterMeasurements.uCurrOffsetW)
					* AMPERES_PER_BIT;
			break;
		default:
			}

	}

	gInverterMeasurements.dmaIndex = shutdownInfo.dmaIndex;
	gInverterMeasurements.faultIndex = shutdownInfo.faultIndex;
}

