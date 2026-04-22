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
#include "stdint.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

#define MEASUREMENT_SIZE 8000

typedef struct {
	//Buffer length
	uint32_t bufferSize;

	//Indexes
	uint32_t dmaIndex;
	uint32_t faultIndex;

	//Time Step
	uint32_t uTimeStepUs;

	//Conversion constants
	float fVoltPerBit;
	float fAmperePerbit;

	// Current offsets
	volatile uint16_t uCurrOffsetU;
	volatile uint16_t uCurrOffsetV;
	volatile uint16_t uCurrOffsetW;

	// Voltage & sensor measurements
	uint32_t uDcLinkVoltage[3*MEASUREMENT_SIZE];
	uint32_t uPhaseSens[3*MEASUREMENT_SIZE];
	uint32_t uCurrSens[3*MEASUREMENT_SIZE];

} inverterMeasurementsTypeDef_t;

extern inverterMeasurementsTypeDef_t gInverterMeasurements;

typedef enum
{
	OC = 0,		// Open Circuit
	ASC_LOW = 1,	// Active Short Circuit Low Side
	ASC_HIGH = 2	// Active Short Circuit High Side. Careful due to bootstrap it will persist for a limited time!!!
}shutdownType_t;

extern shutdownType_t gShutdownType;

extern volatile float fAngleEl;

extern volatile float fDutyD;
extern volatile float fDutyQ;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a,b,c) MAX(MAX(a,b),c)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a,b,c) MIN(MIN(a,b),c)

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
#define LED_Fault_Pin GPIO_PIN_2
#define LED_Fault_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

#define VOLTS_PER_BIT 12.0f/790.0f
#define AMPERES_PER_BIT 80.0f/4096.0f
#define BITS_PER_AMPERE 4096.0f/80.0f
#define BITS_PER_VOLT 790.0f/12.0f
#define DEGREE_PER_BIT 360.0f/16384.0f

#define DEGREES_PER_BIT 2*PI/65536.0f

#define ARR_VAL 6874

#define POLE_PAIR 4
#define ANGLE_OFFSET 20801

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
