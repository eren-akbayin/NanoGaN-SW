/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32h7xx_it.c
 * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32h7xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "shutdown.h"
#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CURRENT_DMA_NDTR 0x4002002c //DMA! Steram 1 NDTR

#define VOLTAGE_DMA_NDTR 0x40020414 //DMA2 Stream 0 NDTR

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

volatile uint32_t uNumberOfNops = 500;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
	/* USER CODE BEGIN NonMaskableInt_IRQn 0 */

	/* USER CODE END NonMaskableInt_IRQn 0 */
	/* USER CODE BEGIN NonMaskableInt_IRQn 1 */
	while (1)
	{
	}
	/* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
	/* USER CODE BEGIN HardFault_IRQn 0 */

	/* USER CODE END HardFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_HardFault_IRQn 0 */
		/* USER CODE END W1_HardFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
	/* USER CODE BEGIN MemoryManagement_IRQn 0 */

	/* USER CODE END MemoryManagement_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
		/* USER CODE END W1_MemoryManagement_IRQn 0 */
	}
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
	/* USER CODE BEGIN BusFault_IRQn 0 */

	/* USER CODE END BusFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_BusFault_IRQn 0 */
		/* USER CODE END W1_BusFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
	/* USER CODE BEGIN UsageFault_IRQn 0 */

	/* USER CODE END UsageFault_IRQn 0 */
	while (1)
	{
		/* USER CODE BEGIN W1_UsageFault_IRQn 0 */
		/* USER CODE END W1_UsageFault_IRQn 0 */
	}
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
	/* USER CODE BEGIN DebugMonitor_IRQn 0 */

	/* USER CODE END DebugMonitor_IRQn 0 */
	/* USER CODE BEGIN DebugMonitor_IRQn 1 */

	/* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles ADC1 and ADC2 global interrupts.
 */
void ADC_IRQHandler(void)
{
	/* USER CODE BEGIN ADC_IRQn 0 */

	switch (gShutdownType)
	{
	case (OC):
		TIM1->BDTR &= ~TIM_BDTR_MOE;
		gateDriveShutdown();
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

	for (uint32_t i = 0; i < uNumberOfNops; i++)
			{
		__NOP();
	}

	HAL_TIM_Base_Stop(&htim2);

	getShutdownInfo(CURRENT, 3 * MEASUREMENT_SIZE - *(volatile uint32_t*) CURRENT_DMA_NDTR);

	HAL_GPIO_WritePin(LED_Fault_GPIO_Port, LED_Fault_Pin, GPIO_PIN_SET);
	/* USER CODE END ADC_IRQn 0 */
	HAL_ADC_IRQHandler(&hadc1);
	HAL_ADC_IRQHandler(&hadc2);
	/* USER CODE BEGIN ADC_IRQn 1 */

	/* USER CODE END ADC_IRQn 1 */
}

/**
 * @brief This function handles TIM4 global interrupt.
 */
void TIM4_IRQHandler(void)
{
	/* USER CODE BEGIN TIM4_IRQn 0 */

	/* USER CODE END TIM4_IRQn 0 */
	HAL_TIM_IRQHandler(&htim4);
	/* USER CODE BEGIN TIM4_IRQn 1 */

	/* USER CODE END TIM4_IRQn 1 */
}

/**
 * @brief This function handles SPI2 global interrupt.
 */
void SPI2_IRQHandler(void)
{
	/* USER CODE BEGIN SPI2_IRQn 0 */

	/* USER CODE END SPI2_IRQn 0 */
	HAL_SPI_IRQHandler(&hspi2);
	/* USER CODE BEGIN SPI2_IRQn 1 */

	/* USER CODE END SPI2_IRQn 1 */
}

/**
 * @brief This function handles TIM6 global interrupt, DAC1_CH1 and DAC1_CH2 underrun error interrupts.
 */
void TIM6_DAC_IRQHandler(void)
{
	/* USER CODE BEGIN TIM6_DAC_IRQn 0 */

	/* USER CODE END TIM6_DAC_IRQn 0 */
	HAL_TIM_IRQHandler(&htim6);
	/* USER CODE BEGIN TIM6_DAC_IRQn 1 */

	/* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
 * @brief This function handles ADC3 global interrupt.
 */
void ADC3_IRQHandler(void)
{
	/* USER CODE BEGIN ADC3_IRQn 0 */

	TIM1->BDTR &= ~TIM_BDTR_MOE;

	//Keep the measurement going for little bit longer in case of shutdown creates a bigger problem

	for (uint32_t i = 0; i < uNumberOfNops; i++)
			{
		__NOP();
	}

	HAL_TIM_Base_Stop(&htim2);

	gateDriveShutdown();

	getShutdownInfo(VOLTAGE, 3 * MEASUREMENT_SIZE - *(volatile uint32_t*) VOLTAGE_DMA_NDTR);

	HAL_GPIO_WritePin(LED_Fault_GPIO_Port, LED_Fault_Pin, GPIO_PIN_SET);

	/* USER CODE END ADC3_IRQn 0 */
	HAL_ADC_IRQHandler(&hadc3);
	/* USER CODE BEGIN ADC3_IRQn 1 */

	/* USER CODE END ADC3_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
