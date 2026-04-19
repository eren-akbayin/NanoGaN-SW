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

ADC_AnalogWDGConfTypeDef AnalogWDGConfig_Currents =
{ 0 };
ADC_AnalogWDGConfTypeDef AnalogWDGConfig_VoltageDc =
{ 0 };

shutdownInfoTypeDef_t shutdownInfo;

static int32_t calibrateOffset(uint32_t *pData, uint32_t len, uint8_t offset)
{

	int64_t sum = 0;

	uint32_t count = 0;

	for (uint32_t i = offset; i < len; i = i + 3)
	{
		sum += (int32_t) pData[i];
		count++;
	}

	return (int32_t) (sum / count);
}

void calibrateSensorsSetShutdowns(float i_max, float u_min, float u_max)
{

	HAL_ADC_Stop_DMA(&hadc1);

	HAL_ADC_Stop_DMA(&hadc2);

	HAL_ADC_Stop_DMA(&hadc3);

	HAL_TIM_Base_Stop(&htim2);

	HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY,
	ADC_SINGLE_ENDED);

	HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET_LINEARITY,
	ADC_SINGLE_ENDED);

	HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY,
	ADC_SINGLE_ENDED);

	HAL_ADC_Start_DMA(&hadc2, uCurrSens, MEASUREMENT_LENGTH);

	HAL_TIM_Base_Start(&htim2);

	tx_thread_sleep(100);

	HAL_TIM_Base_Stop(&htim2);

	HAL_ADC_Stop_DMA(&hadc2);

	uCurrOffsetU = calibrateOffset(uCurrSens, MEASUREMENT_LENGTH, 0);
	uCurrOffsetV = calibrateOffset(uCurrSens, MEASUREMENT_LENGTH, 1);
	uCurrOffsetW = calibrateOffset(uCurrSens, MEASUREMENT_LENGTH, 2);

	uint32_t uAwdHigh = MIN3(uCurrOffsetU, uCurrOffsetV, uCurrOffsetW)
			+ (uint32_t) (BITS_PER_CURRENT * i_max);
	uint32_t uAwdLow = MAX3(uCurrOffsetU, uCurrOffsetV, uCurrOffsetW)
			- (uint32_t) (BITS_PER_CURRENT * i_max);

	AnalogWDGConfig_Currents.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
	AnalogWDGConfig_Currents.WatchdogMode = ADC_ANALOGWATCHDOG_ALL_REG;
	AnalogWDGConfig_Currents.ITMode = ENABLE;
	AnalogWDGConfig_Currents.HighThreshold = uAwdHigh;
	AnalogWDGConfig_Currents.LowThreshold = uAwdLow;

	if (HAL_ADC_AnalogWDGConfig(&hadc2, &AnalogWDGConfig_Currents) != HAL_OK)
	{
		Error_Handler();
	}

	AnalogWDGConfig_VoltageDc.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
	AnalogWDGConfig_VoltageDc.WatchdogMode = ADC_ANALOGWATCHDOG_ALL_REG;
	AnalogWDGConfig_VoltageDc.ITMode = ENABLE;
	AnalogWDGConfig_VoltageDc.HighThreshold = (uint32_t) (u_max
			* BITS_PER_VOLTAGE);
	AnalogWDGConfig_VoltageDc.LowThreshold = (uint32_t) (u_min
			* BITS_PER_VOLTAGE);
	AnalogWDGConfig_VoltageDc.FilteringConfig = ADC3_AWD_FILTERING_NONE;
	if (HAL_ADC_AnalogWDGConfig(&hadc3, &AnalogWDGConfig_VoltageDc) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_ADC_Start_DMA(&hadc1, uPhaseSens, MEASUREMENT_LENGTH);

	HAL_ADC_Start_DMA(&hadc2, uCurrSens, MEASUREMENT_LENGTH);

	HAL_ADC_Start_DMA(&hadc3, uDcLinkVoltage, MEASUREMENT_LENGTH);

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
	TIM1->CCR3 = 6875;
	TIM1->CCR2 = 6875;
	TIM1->CCR1 = 6875;
}

void getShutdownInfo(measurementType_t measurementType, uint32_t dmaIndex)
{
	memset(&shutdownInfo, 0, sizeof(shutdownInfo));
	shutdownInfo.measurementType = measurementType;
	shutdownInfo.dmaIndex = dmaIndex;
	if (measurementType == VOLTAGE)
	{
		shutdownInfo.measuredRaw = ADC3->DR;
		shutdownInfo.measured = (float) (shutdownInfo.measuredRaw)
				* VOLTAGE_PER_BITS;
		shutdownInfo.thresholds.lowerThresholdRaw =
				AnalogWDGConfig_VoltageDc.LowThreshold;
		shutdownInfo.thresholds.upperThresholdRaw =
				AnalogWDGConfig_VoltageDc.HighThreshold;
		shutdownInfo.thresholds.lowerThreshold =
				(float) (shutdownInfo.thresholds.lowerThresholdRaw)
						* VOLTAGE_PER_BITS;
		shutdownInfo.thresholds.upperThreshold =
				(float) (shutdownInfo.thresholds.upperThresholdRaw)
						* VOLTAGE_PER_BITS;
		if (shutdownInfo.measuredRaw
				>= shutdownInfo.thresholds.upperThresholdRaw)
		{
			shutdownInfo.shutdownType = OVER_VOLTAGE_SHUTDOWN;
		}
		else
		{
			shutdownInfo.shutdownType = UNDER_VOLTAGE_SHUTDOWN;
		}
	}
	else if (measurementType == CURRENT)
	{
		shutdownInfo.thresholds.lowerThresholdRaw =
				AnalogWDGConfig_Currents.LowThreshold;
		shutdownInfo.thresholds.upperThresholdRaw =
				AnalogWDGConfig_Currents.HighThreshold;

		uint16_t i = dmaIndex;

		do
		{
			if (uCurrSens[i] < shutdownInfo.thresholds.lowerThresholdRaw
					|| uCurrSens[i] > shutdownInfo.thresholds.upperThresholdRaw)
				break;

			i--;

			if (i < 0)
				i = MEASUREMENT_LENGTH - 1;

		} while (i != dmaIndex);

		shutdownInfo.faultIndex = i;

		shutdownInfo.measuredRaw = uCurrSens[shutdownInfo.faultIndex];

		switch (shutdownInfo.faultIndex % 3)
		{
		case 0:
			shutdownInfo.shutdownType = OVER_CURRENT_SHUTDOWN_PHASE_U;
			shutdownInfo.measured =
					(float) ((int32_t) uCurrSens[shutdownInfo.faultIndex]
							- (int32_t) uCurrOffsetU) * -CURRENT_PER_BITS;
			shutdownInfo.thresholds.lowerThreshold =
					-(float) (AnalogWDGConfig_Currents.HighThreshold
							- uCurrOffsetV) * CURRENT_PER_BITS;
			shutdownInfo.thresholds.upperThreshold = (float) (uCurrOffsetV
					- AnalogWDGConfig_Currents.LowThreshold) * CURRENT_PER_BITS;
			break;
		case 1:
			shutdownInfo.shutdownType = OVER_CURRENT_SHUTDOWN_PHASE_V;
			shutdownInfo.measured =
					(float) ((int32_t) uCurrSens[shutdownInfo.faultIndex]
							- (int32_t) uCurrOffsetV) * CURRENT_PER_BITS;
			shutdownInfo.thresholds.lowerThreshold = -(float) (uCurrOffsetV
					- AnalogWDGConfig_Currents.LowThreshold) * CURRENT_PER_BITS;
			shutdownInfo.thresholds.upperThreshold =
					(float) (AnalogWDGConfig_Currents.HighThreshold
							- uCurrOffsetV) * CURRENT_PER_BITS;
			break;
		case 2:
			shutdownInfo.shutdownType = OVER_CURRENT_SHUTDOWN_PHASE_W;
			shutdownInfo.measured =
					(float) ((int32_t) uCurrSens[shutdownInfo.faultIndex]
							- (int32_t) uCurrOffsetW) * CURRENT_PER_BITS;
			shutdownInfo.thresholds.lowerThreshold = -(float) (uCurrOffsetW
					- AnalogWDGConfig_Currents.LowThreshold) * CURRENT_PER_BITS;
			shutdownInfo.thresholds.upperThreshold =
					(float) (AnalogWDGConfig_Currents.HighThreshold
							- uCurrOffsetW) * CURRENT_PER_BITS;
			break;
		default:
		}

	}
}

