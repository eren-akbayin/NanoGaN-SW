/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    app_threadx.c
 * @author  MCD Application Team
 * @brief   ThreadX applicative file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2020-2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
#include "adc.h"
#include "tim.h"
#include "spi.h"
#include "stdlib.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MAX_CURRENT 5.0f

#define VOLTAGE_PER_BITS 12.0f/790.0f
#define CURRENT_PER_BITS 80.0f/4096.0f
#define BITS_PER_CURRENT 4096.0f/80.0f

#define DEGREE_PER_BITS 360.0f/16384.0f

#define MEASUREMENT_LENGTH 150

#define ARR_VAL 27500

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a,b,c) MAX(MAX(a,b),c)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a,b,c) MIN(MIN(a,b),c)

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD tx_app_thread;
/* USER CODE BEGIN PV */

// Fault flag

volatile float fFault = 0;


// DMA Buffers
uint32_t uDcLinkVoltage;
uint32_t uPhaseSens [MEASUREMENT_LENGTH];
uint32_t uCurrSens [MEASUREMENT_LENGTH];

ADC_AnalogWDGConfTypeDef AnalogWDGConfig = {0};

volatile uint16_t uAngleRaw;

// Control
volatile uint32_t uDuty = 0;
volatile uint32_t uArr = 27499;

// HALL Stuff

volatile uint8_t uHallRead = 0;

// Converted Measurement
volatile float fAngle;

volatile float fCurrentU;
volatile float fCurrentV;
volatile float fCurrentW;
volatile float fDcLinkVoltage;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static int32_t calibrateOffset(uint32_t *pData, size_t len, uint8_t offset);
static void calibrateCurrentSensors(float i_max);

/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */

  /* USER CODE END App_ThreadX_MEM_POOL */
  CHAR *pointer;

  /* Allocate the stack for tx nanogan thread  */
  if (tx_byte_allocate(byte_pool, (VOID**) &pointer,
                       TX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  /* Create tx nanogan thread.  */
  if (tx_thread_create(&tx_app_thread, "tx nanogan thread", tx_nanogan_app, 0, pointer,
                       TX_APP_STACK_SIZE, TX_APP_THREAD_PRIO, TX_APP_THREAD_PREEMPTION_THRESHOLD,
                       TX_APP_THREAD_TIME_SLICE, TX_APP_THREAD_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN App_ThreadX_Init */

  /* USER CODE END App_ThreadX_Init */

  return ret;
}
/**
  * @brief  Function implementing the tx_nanogan_app thread.
  * @param  thread_input: Hardcoded to 0.
  * @retval None
  */
void tx_nanogan_app(ULONG thread_input)
{
  /* USER CODE BEGIN tx_nanogan_app */
	calibrateCurrentSensors(MAX_CURRENT);
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);

	// Start DMAs
	HAL_ADC_Start_DMA(&hadc1, uPhaseSens, MEASUREMENT_LENGTH);
	HAL_ADC_Start_DMA(&hadc3, &uDcLinkVoltage , 1);

	// Start Timer for periodic triggering of ADCs
	HAL_TIM_Base_Start_IT(&htim4);

	// Write high to the MOSI to always get compensated angle
	HAL_GPIO_WritePin(SPI2_MOSI_GPIO_Port,SPI2_MOSI_Pin,GPIO_PIN_SET);

	// Wait for DMA buffers to get full
	tx_thread_sleep(100);

	uFault = 0;
	fFault = 0.0f;

	TIM1->CCR3 = 0;

	TIM1->CCR2 = 0;

	TIM1->CCR1 = 0;

	/** Configure Analog WatchDog 1
	*/
	while (1)
	{

		tx_thread_sleep(1);
	}

  /* USER CODE END tx_nanogan_app */
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 2 */

static int32_t calibrateOffset(uint32_t *pData, size_t len, uint8_t offset)
{

    int64_t sum = 0;

    uint8_t count = 0;

    for (size_t i = offset; i < len; i = i + 3)
    {
            sum += (int32_t)pData[i];
            count++;
    }

    return (int32_t)(sum / count);
}

static void calibrateCurrentSensors(float i_max)
{

	HAL_ADC_Stop_DMA(&hadc2);

	HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);

	HAL_ADC_Start_DMA(&hadc2, uCurrSens, MEASUREMENT_LENGTH);

	HAL_TIM_Base_Start(&htim2);

	tx_thread_sleep(100);

	HAL_TIM_Base_Stop(&htim2);

	HAL_ADC_Stop_DMA(&hadc2);

	uint32_t uCurrOffsetU = calibrateOffset(uCurrSens, MEASUREMENT_LENGTH, 0);
	uint32_t uCurrOffsetV = calibrateOffset(uCurrSens, MEASUREMENT_LENGTH, 1);
	uint32_t uCurrOffsetW = calibrateOffset(uCurrSens, MEASUREMENT_LENGTH, 2);

	uint32_t uAwdHigh = MIN3(uCurrOffsetU, uCurrOffsetV, uCurrOffsetW) + (uint32_t)(BITS_PER_CURRENT*i_max);

	uint32_t uAwdLow = MAX3(uCurrOffsetU, uCurrOffsetV, uCurrOffsetW) - (uint32_t)(BITS_PER_CURRENT*i_max);

	AnalogWDGConfig.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
	AnalogWDGConfig.WatchdogMode = ADC_ANALOGWATCHDOG_ALL_REG;
	AnalogWDGConfig.ITMode = ENABLE;
	AnalogWDGConfig.HighThreshold = uAwdHigh;
	AnalogWDGConfig.LowThreshold = uAwdLow;

	if (HAL_ADC_AnalogWDGConfig(&hadc2, &AnalogWDGConfig) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_ADC_Start_DMA(&hadc2, uCurrSens, MEASUREMENT_LENGTH);

	HAL_TIM_Base_Start(&htim2);

}

/* USER CODE END 2 */
