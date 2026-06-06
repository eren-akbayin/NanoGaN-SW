/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

extern uint16_t uAngleMech;

extern volatile uint16_t uAngleRaw;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define ARR_VAL (TIM1->ARR) // Default PWM ARR value.

#define PROFILER_START()    do { TIM5->CNT = 0; TIM5->CR1 |=  TIM_CR1_CEN; } while(0)
#define PROFILER_STOP()     do {                TIM5->CR1 &= ~TIM_CR1_CEN; } while(0)
#define PROFILER_READ()     (TIM5->CNT)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define HALL_A_Pin GPIO_PIN_13
#define HALL_A_GPIO_Port GPIOC
#define HALL_B_Pin GPIO_PIN_14
#define HALL_B_GPIO_Port GPIOC
#define HALL_C_Pin GPIO_PIN_15
#define HALL_C_GPIO_Port GPIOC
#define U_DC_SENS_Pin GPIO_PIN_2
#define U_DC_SENS_GPIO_Port GPIOC
#define U_PHASE_SENS_W_Pin GPIO_PIN_0
#define U_PHASE_SENS_W_GPIO_Port GPIOA
#define U_PHASE_SENS_V_Pin GPIO_PIN_1
#define U_PHASE_SENS_V_GPIO_Port GPIOA
#define U_PHASE_SENS_U_Pin GPIO_PIN_2
#define U_PHASE_SENS_U_GPIO_Port GPIOA
#define GATE_WN_Pin GPIO_PIN_8
#define GATE_WN_GPIO_Port GPIOE
#define GATE_WP_Pin GPIO_PIN_9
#define GATE_WP_GPIO_Port GPIOE
#define GATE_VN_Pin GPIO_PIN_10
#define GATE_VN_GPIO_Port GPIOE
#define GATE_VP_Pin GPIO_PIN_11
#define GATE_VP_GPIO_Port GPIOE
#define GATE_UN_Pin GPIO_PIN_12
#define GATE_UN_GPIO_Port GPIOE
#define GATE_UP_Pin GPIO_PIN_13
#define GATE_UP_GPIO_Port GPIOE
#define SPI2_MOSI_Pin GPIO_PIN_15
#define SPI2_MOSI_GPIO_Port GPIOB
#define USR_BTN2_Pin GPIO_PIN_9
#define USR_BTN2_GPIO_Port GPIOA
#define USR_BTN1_Pin GPIO_PIN_10
#define USR_BTN1_GPIO_Port GPIOA
#define LED_FAULT_Pin GPIO_PIN_2
#define LED_FAULT_GPIO_Port GPIOD
#define LED_ACTIVE_Pin GPIO_PIN_5
#define LED_ACTIVE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

#define SCRUTINY_CDC  0 //change according to tusb_descriptiors.c
#define USER_CDC      1 //change according to tusb_descriptiors.c

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
