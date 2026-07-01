#include "measurement.h"
#include "adc.h"
#include "cmsis_os2.h"
#include "tim.h"
#include "spi.h"
#include "main.h"
#include "cmsis_os.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a,b,c) MAX(MAX(a,b),c)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a,b,c) MIN(MIN(a,b),c)

// Analog watchdog config values

ADC_AnalogWDGConfTypeDef AnalogWDGConfig_Currents = {0};
ADC_AnalogWDGConfTypeDef AnalogWDGConfig_VoltageDc = {0};

//Measurement data structs
inverterMeasurementsTypeDef_t gInverterMeasurements __attribute__((section(".dma_data"))) = { 0 };
shutdownInfoTypeDef_t gShutdownInfo;
shutdownType_t gShutdownType = OC;

//Function prototypes

static int32_t calibrateOffset(uint32_t *pData, uint32_t len, uint8_t offset);

//Shutdown settings

#define NUMBER_OF_NOPS 0 //This is how much we keep ADCs going after shutdown.

// id and iq currents
float fCurrD = 0.0f;
float fCurrQ = 0.0f;

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

void stopMeasurements(void)
{

	HAL_TIM_Base_Stop(&htim2);
	HAL_SPI_DMAStop(&hspi2);

	HAL_ADC_Stop_DMA(&hadc1);
	HAL_ADC_Stop_DMA(&hadc2);
	HAL_ADC_Stop_DMA(&hadc3);

	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);

}

void startMeasurements(void)
{

	HAL_ADC_Start_DMA(&hadc1, gInverterMeasurements.uPhaseSens, 3 * 10);
	HAL_ADC_Start_DMA(&hadc2, gInverterMeasurements.uCurrSens, 3 * 10);
	HAL_ADC_Start_DMA(&hadc3, gInverterMeasurements.uDcLinkVoltage, 10);

	HAL_SPI_Receive_DMA(&hspi2, (uint8_t*)&uAngleRaw, 1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	HAL_TIM_Base_Start(&htim2);

}

void calibrateSensorsSetShutdowns(float i_max, float u_min, float u_max)
{

	// Write high to the MOSI to always get compensated angle

	stopMeasurements();

	HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET_LINEARITY, ADC_DIFFERENTIAL_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);

	startMeasurements();

	osDelay(100);

	stopMeasurements();

	gInverterMeasurements.uCurrOffsetU = calibrateOffset(gInverterMeasurements.uCurrSens, 3 * 10, 0);
	gInverterMeasurements.uCurrOffsetV = calibrateOffset(gInverterMeasurements.uCurrSens, 3 * 10, 1);
	gInverterMeasurements.uCurrOffsetW = calibrateOffset(gInverterMeasurements.uCurrSens, 3 * 10, 2);

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

	startMeasurements();

}



void stopGateDrive(void)
{
	TIM1->BDTR &= ~TIM_BDTR_MOE;
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
	TIM1->CCR3 = ARR_VAL >> 1;
	TIM1->CCR2 = ARR_VAL >> 1;
	TIM1->CCR1 = ARR_VAL >> 1;
}

void shutdownGateDrive(void)
{
	switch (gShutdownType)
	{
	case (OC):
		stopGateDrive();
	case (ASC_LOW):
		TIM1->CCR3 = 0;
		TIM1->CCR2 = 0;
		TIM1->CCR1 = 0;
	case (ASC_HIGH):
		TIM1->CCR3 = ARR_VAL;
		TIM1->CCR2 = ARR_VAL;
		TIM1->CCR1 = ARR_VAL;
	}

	//Keep the measurement going for little bit longer in case of shutdown creates a bigger problem

	for (uint32_t i = 0; i < NUMBER_OF_NOPS; i++)
			{
		__NOP();
	}

	HAL_TIM_Base_Stop(&htim2); //We stop timer 2 here because this is what we are using to trigger ADC conversions.

}

void startGateDrive(void)
{
	TIM1->CCR3 = ARR_VAL >> 1;
	TIM1->CCR2 = ARR_VAL >> 1;
	TIM1->CCR1 = ARR_VAL >> 1;

	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void clearShutdownInfo()
{
	// Set everything to default

	gShutdownInfo.measuredRaw = 0;
	gShutdownInfo.measured = 0.0f;

	gShutdownInfo.measurementType = NONE;
	
	gShutdownInfo.thresholds.lowerThresholdRaw = 0;
	gShutdownInfo.thresholds.upperThresholdRaw = 0;
	gShutdownInfo.thresholds.lowerThreshold = 0.0f;
	gShutdownInfo.thresholds.upperThreshold = 0.0f;

	gShutdownInfo.faultType = NO_FAULT;
	gShutdownInfo.dmaIndexPhaseCurrents = 0;
	gShutdownInfo.dmaIndexPhaseVoltages = 0;
	gShutdownInfo.dmaIndexAngle = 0;
	gShutdownInfo.faultIndex = 0;
}

void getShutdownInfo(measurementType_t measurementType)
{
	//First make sure there is nothing left from any previous shutdowns
	clearShutdownInfo();

	//Start collecting information
	gShutdownInfo.measurementType = measurementType;
	gShutdownInfo.dmaIndexPhaseCurrents = 3 * 10 - *(volatile uint32_t*) PHASE_CURRENT_DMA_NDTR;
	gShutdownInfo.dmaIndexPhaseVoltages = 3 * 10 - *(volatile uint32_t*) PHASE_VOLTAGE_DMA_NDTR;
	gShutdownInfo.dmaIndexDCVoltage = 10 - *(volatile uint32_t*) DC_VOLTAGE_DMA_NDTR;

	uint16_t i;

	if (gShutdownInfo.measurementType == VOLTAGE)
			{
		i = 3 * 10 - *(volatile uint32_t*) DC_VOLTAGE_DMA_NDTR;

		gShutdownInfo.thresholds.lowerThresholdRaw = AnalogWDGConfig_VoltageDc.LowThreshold;
		gShutdownInfo.thresholds.upperThresholdRaw = AnalogWDGConfig_VoltageDc.HighThreshold;

		do
		{
			if (gInverterMeasurements.uDcLinkVoltage[i] < gShutdownInfo.thresholds.lowerThresholdRaw)
					{
				gShutdownInfo.faultType = UNDER_VOLTAGE_FAULT;
				break;
			}
			else if (gInverterMeasurements.uDcLinkVoltage[i] > gShutdownInfo.thresholds.upperThresholdRaw)
					{
				gShutdownInfo.faultType = OVER_VOLTAGE_FAULT;
				break;
			}

			i++;

			if (i == 3 * 10)
				i = 0;

		} while (i != gShutdownInfo.dmaIndexDCVoltage);

		gShutdownInfo.faultIndex = i;

		gShutdownInfo.measuredRaw = gInverterMeasurements.uDcLinkVoltage[gShutdownInfo.faultIndex];
		gShutdownInfo.measured = (float)(gShutdownInfo.measuredRaw) * VOLTS_PER_BIT;
		gShutdownInfo.thresholds.lowerThreshold = (float)(gShutdownInfo.thresholds.lowerThresholdRaw) * VOLTS_PER_BIT;
		gShutdownInfo.thresholds.upperThreshold = (float)(gShutdownInfo.thresholds.upperThresholdRaw) * VOLTS_PER_BIT;
	}
	else if (gShutdownInfo.measurementType == CURRENT)
			{
		i = 3 * 10 - *(volatile uint32_t*) PHASE_CURRENT_DMA_NDTR;
		gShutdownInfo.thresholds.lowerThresholdRaw = AnalogWDGConfig_Currents.LowThreshold;
		gShutdownInfo.thresholds.upperThresholdRaw = AnalogWDGConfig_Currents.HighThreshold;

		do
		{
			if (gInverterMeasurements.uCurrSens[i] < gShutdownInfo.thresholds.lowerThresholdRaw
					|| gInverterMeasurements.uCurrSens[i] > gShutdownInfo.thresholds.upperThresholdRaw)
				break;

			i++;

			if (i == 3 * 10)
				i = 0;

		} while (i != gShutdownInfo.dmaIndexPhaseCurrents);

		gShutdownInfo.faultIndex = i;
		gShutdownInfo.measuredRaw = gInverterMeasurements.uCurrSens[gShutdownInfo.faultIndex];

		switch (gShutdownInfo.faultIndex % 3)
		{
		case 0:
			gShutdownInfo.faultType = OVER_CURRENT_FAULT_PHASE_U;
			gShutdownInfo.measured = (float)((int32_t)gInverterMeasurements.uCurrSens[gShutdownInfo.faultIndex]
					- (int32_t)gInverterMeasurements.uCurrOffsetU) * -AMPERES_PER_BIT;
			gShutdownInfo.thresholds.lowerThreshold = -(float)(AnalogWDGConfig_Currents.HighThreshold
					- gInverterMeasurements.uCurrOffsetV) * AMPERES_PER_BIT;
			gShutdownInfo.thresholds.upperThreshold = (float)(gInverterMeasurements.uCurrOffsetV
					- AnalogWDGConfig_Currents.LowThreshold) * AMPERES_PER_BIT;
			break;
		case 1:
			gShutdownInfo.faultType =
					OVER_CURRENT_FAULT_PHASE_V;
			gShutdownInfo.measured = (float)((int32_t)gInverterMeasurements.uCurrSens[gShutdownInfo.faultIndex]
					- (int32_t)gInverterMeasurements.uCurrOffsetV) * AMPERES_PER_BIT;
			gShutdownInfo.thresholds.lowerThreshold = -(float)(gInverterMeasurements.uCurrOffsetV - AnalogWDGConfig_Currents.LowThreshold)
					* AMPERES_PER_BIT;
			gShutdownInfo.thresholds.upperThreshold = (float)(AnalogWDGConfig_Currents.HighThreshold - gInverterMeasurements.uCurrOffsetV)
					* AMPERES_PER_BIT;
			break;
		case 2:
			gShutdownInfo.faultType = OVER_CURRENT_FAULT_PHASE_W;
			gShutdownInfo.measured = (float)((int32_t)gInverterMeasurements.uCurrSens[gShutdownInfo.faultIndex]
					- (int32_t)gInverterMeasurements.uCurrOffsetW) * AMPERES_PER_BIT;
			gShutdownInfo.thresholds.lowerThreshold = -(float)(gInverterMeasurements.uCurrOffsetW - AnalogWDGConfig_Currents.LowThreshold)
					* AMPERES_PER_BIT;
			gShutdownInfo.thresholds.upperThreshold = (float)(AnalogWDGConfig_Currents.HighThreshold - gInverterMeasurements.uCurrOffsetW)
					* AMPERES_PER_BIT;
			break;
		default:
			break;
		}

	}

}

