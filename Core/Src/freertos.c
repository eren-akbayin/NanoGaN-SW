/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adc.h"
#include "tim.h"
#include "spi.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VOLTAGE_PER_BITS 12.0f/790.0f
#define CURRENT_PER_BITS 80.0f/4096.0f

#define DEGREE_PER_BITS 360.0f/16384.0f

#define MEASUREMENT_LENGTH 100

#define ARR_VAL 27500

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
int32_t calibrateOffset(uint32_t *pData, size_t len);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */

// DMA Buffers
uint32_t uDcLinkVoltage;
uint32_t uPhaseSens [MEASUREMENT_LENGTH];
uint32_t uCurrSens [MEASUREMENT_LENGTH];

volatile uint16_t uAngleRaw;

// Control
volatile uint32_t uDuty = 0;

// Converted Measurement
volatile float fAngle;
volatile float fCurrent;

volatile int32_t uCurrOffset;

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

	// Calibrate the ADCs
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);

	// Start DMAs
	HAL_ADC_Start_DMA(&hadc1, uPhaseSens, MEASUREMENT_LENGTH);
	HAL_ADC_Start_DMA(&hadc2, uCurrSens, MEASUREMENT_LENGTH);
	HAL_ADC_Start_DMA(&hadc3, &uDcLinkVoltage , 1);

	// Wait for buffers to calibrate

	osDelay(1);

	// Start Timer for periodic triggering of ADCs
	HAL_TIM_Base_Start(&htim2);

	// Write high to the MOSI to always get compensated angle
	HAL_GPIO_WritePin(SPI2_MOSI_GPIO_Port,SPI2_MOSI_Pin,GPIO_PIN_SET);

	// Wait for DMA buffers to get full

	// Calculate the current offset before enabling the phases
	uCurrOffset = calibrateOffset(uCurrSens, MEASUREMENT_LENGTH);

	// Start PWMs to go to active short circuit
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

	// Make sure that channel 1 is always low
	TIM1->CCR3 = 0;

	HAL_SPI_Receive_DMA(&hspi2, (uint8_t*) &uAngleRaw, 1);
  /* Infinite loop */
  for(;;)
  {
	  if (uDcLinkVoltage<750 || uDcLinkVoltage > 1600){
		HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
		HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
		HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_3);
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
	  }

	  //Calculate measurement values online
	  fAngle = (float)(uAngleRaw & 0x3FFF) * DEGREE_PER_BITS;
	  fCurrent = (float)((int32_t)uCurrSens[0] - uCurrOffset)* CURRENT_PER_BITS;

	  //Write the duty cycle
	  TIM1->CCR1 = uDuty;
	  osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
int32_t calibrateOffset(uint32_t *pData, size_t len)
{

    int64_t sum = 0;

    for (size_t i = 0; i < len; i++)
    {
            sum += (int32_t)pData[i];
    }

    return (int32_t)(sum / len);
}

/* USER CODE END Application */

