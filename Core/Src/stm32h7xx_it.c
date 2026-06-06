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
#include "tusb.h"
#include "measurement.h"
#include "core_tasks.h"
#include "cordic.h"
#include "scrutiny_integration.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
uint32_t uProfileLength = 0;

uint16_t uAngleManual = 0;
uint16_t uIncrementManual = 0;

uint16_t uAngleSelected = 0;
uint8_t uAngleSelection = 0;

float fCurrent[3] = {0.0f, 0.0f, 0.0f};

uint32_t uWriteData = 0;
uint32_t uRawData = 0;

volatile float fCosAlpha;
volatile float fSinAlpha;

volatile float fDutyD = 0;
volatile float fDutyQ = 0;

uint32_t uDTC[3]; 

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
extern TIM_HandleTypeDef htim1;
extern PCD_HandleTypeDef hpcd_USB_OTG_HS;
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
  shutdownGateDrive();
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
  shutdownGateDrive();
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
  shutdownGateDrive();
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
  shutdownGateDrive();
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
  shutdownGateDrive();
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
  shutdownGateDrive();
  inverter_fsm_post_fault(CURRENT);
  /* USER CODE END ADC_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  HAL_ADC_IRQHandler(&hadc2);
  /* USER CODE BEGIN ADC_IRQn 1 */

  /* USER CODE END ADC_IRQn 1 */
}

/**
  * @brief This function handles TIM1 update interrupt.
  */
void TIM1_UP_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_IRQn 0 */

  PROFILER_START();

  uAngleMech = (uint16_t)(-(uint16_t)((uAngleRaw & 0x3FFF) << 2));

  gInverterMeasurements.uAngleEl = (uint16_t)((uint32_t)uAngleMech * gInverterMeasurements.uPolePair) + gInverterMeasurements.uAngleOffset;

  switch (uAngleSelection)
  {
    case 0:
      uAngleSelected = uAngleManual;
      break;
    case 1:
      uAngleSelected = gInverterMeasurements.uAngleEl;
      break;
    default:
      uAngleSelected = uAngleManual;
      break;
  }

	uint32_t uWriteData = 0x7FFF0000 | (uint32_t)uAngleSelected;

	uint32_t uRawData;

	hcordic.Instance->WDATA = uWriteData;

	uRawData = hcordic.Instance->RDATA;

	fCosAlpha = (float)(int16_t)(uRawData) * (1.0f / 32767.0f);
	fSinAlpha = (float)(int16_t)((uRawData >> 16)) * (1.0f / 32767.0f);

	float Valpha = fDutyD * fCosAlpha - fDutyQ * fSinAlpha;
	float Vbeta = fDutyD * fSinAlpha + fDutyQ * fCosAlpha;

	// Inverse Clarke → 3-phase duty cycles
	uDTC[0] = (uint32_t)((Valpha) * (ARR_VAL>>1) + (ARR_VAL>>1));
	uDTC[1]  = (uint32_t)((-Valpha * 0.5f + Vbeta * 0.866025f) * (ARR_VAL>>1) + (ARR_VAL>>1));
	uDTC[2]  = (uint32_t)((-Valpha * 0.5f - Vbeta * 0.866025f) * (ARR_VAL>>1) + (ARR_VAL>>1));

	TIM1->CCR3 = uDTC[0];

	TIM1->CCR2 = uDTC[1];

	TIM1->CCR1 = uDTC[2];

  fCurrent[0] = (float)((int32_t)gInverterMeasurements.uCurrSens[0] - (int32_t)gInverterMeasurements.uCurrOffsetU)
      * -AMPERES_PER_BIT;
  fCurrent[1] = (float)((int32_t)gInverterMeasurements.uCurrSens[1] - (int32_t)gInverterMeasurements.uCurrOffsetV)
      * AMPERES_PER_BIT;
  fCurrent[2] = (float)((int32_t)gInverterMeasurements.uCurrSens[2] - (int32_t)gInverterMeasurements.uCurrOffsetW)
      * AMPERES_PER_BIT;

	uAngleManual = uAngleManual + uIncrementManual;

  scrutiny_loop_process(100U);

  /* USER CODE END TIM1_UP_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_IRQn 1 */
  PROFILER_STOP();

  uProfileLength = PROFILER_READ();
  /* USER CODE END TIM1_UP_IRQn 1 */
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
  * @brief This function handles USB On The Go HS global interrupt.
  */
void OTG_HS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_HS_IRQn 0 */

  tud_int_handler(0);

  return;

  /* USER CODE END OTG_HS_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);
  /* USER CODE BEGIN OTG_HS_IRQn 1 */

  /* USER CODE END OTG_HS_IRQn 1 */
}

/**
  * @brief This function handles ADC3 global interrupt.
  */
void ADC3_IRQHandler(void)
{
  /* USER CODE BEGIN ADC3_IRQn 0 */
  shutdownGateDrive();
  inverter_fsm_post_fault(VOLTAGE);
  /* USER CODE END ADC3_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc3);
  /* USER CODE BEGIN ADC3_IRQn 1 */

  /* USER CODE END ADC3_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
