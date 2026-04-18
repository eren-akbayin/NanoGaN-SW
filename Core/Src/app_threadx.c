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
#include "main.h"
#include "stdint.h"
#include "adc.h"
#include "tim.h"
#include "spi.h"
#include "stdlib.h"
#include "usart.h"
#include "shutdown.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MAX_CURRENT 3.0f
#define MIN_VOLTAGE 12.0f
#define MAX_VOLTAGE 15.0f

#define ARR_VAL 27500

#define TRACEX_BUFFER_SIZE 64000

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD tx_app_thread;
/* USER CODE BEGIN PV */

uint8_t tracex_buffer[TRACEX_BUFFER_SIZE] __attribute__ ((section (".trace")));

// DMA Buffers

volatile uint16_t uAngleRaw;

// Control
volatile uint32_t uDuty = 0;
volatile uint32_t uArr = 6875;

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

  /* Allocate the stack for tx nanogan fsm thread  */
  if (tx_byte_allocate(byte_pool, (VOID**) &pointer,
                       TX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  /* Create tx nanogan fsm thread.  */
  if (tx_thread_create(&tx_app_thread, "tx nanogan fsm thread", tx_nanogan_fsm_app, 0, pointer,
                       TX_APP_STACK_SIZE, TX_APP_THREAD_PRIO, TX_APP_THREAD_PREEMPTION_THRESHOLD,
                       TX_APP_THREAD_TIME_SLICE, TX_APP_THREAD_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN App_ThreadX_Init */
	tx_trace_enable(&tracex_buffer, TRACEX_BUFFER_SIZE, 30);
  /* USER CODE END App_ThreadX_Init */

  return ret;
}
/**
  * @brief  Function implementing the tx_nanogan_fsm_app thread.
  * @param  thread_input: Hardcoded to 0.
  * @retval None
  */
void tx_nanogan_fsm_app(ULONG thread_input)
{
  /* USER CODE BEGIN tx_nanogan_fsm_app */

	calibrateSensorsSetShutdowns(MAX_CURRENT, MIN_VOLTAGE, MAX_VOLTAGE);

	//HAL_TIM_Base_Start_IT(&htim4);

	// Write high to the MOSI to always get compensated angle
	HAL_GPIO_WritePin(SPI2_MOSI_GPIO_Port, SPI2_MOSI_Pin, GPIO_PIN_SET);

	// Wait for DMA buffers to get full

	TIM1->CCR3 = 0;

	TIM1->CCR2 = 0;

	TIM1->CCR1 = 0;

	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

	/** Configure Analog WatchDog 1
	 */
	while (1)
	{
		fCurrentU = (float) ((int32_t) uCurrSens[0] - (int32_t) uCurrOffsetU)
				* -CURRENT_PER_BITS;
		fCurrentV = (float) ((int32_t) uCurrSens[1] - (int32_t) uCurrOffsetV)
				* CURRENT_PER_BITS;
		fCurrentW = (float) ((int32_t) uCurrSens[2] - (int32_t) uCurrOffsetW)
				* CURRENT_PER_BITS;
		fDcLinkVoltage = (float) (uDcLinkVoltage) * VOLTAGE_PER_BITS;

		TIM1->CCR3 = uDuty;
		tx_thread_sleep(1);
	}

  /* USER CODE END tx_nanogan_fsm_app */
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */
	const char *msg = "Kernel starting!\r\n";

	HAL_UART_Transmit(&huart4, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

	msg = "Kernel has failed to start!\r\n";

	HAL_UART_Transmit(&huart4, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
