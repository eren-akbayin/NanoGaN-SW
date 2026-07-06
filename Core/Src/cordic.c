/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    cordic.c
  * @brief   This file provides code for the configuration
  *          of the CORDIC instances.
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
#include "cordic.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

CORDIC_HandleTypeDef hcordic;

/* CORDIC init function */
void MX_CORDIC_Init(void)
{

  /* USER CODE BEGIN CORDIC_Init 0 */

  /* USER CODE END CORDIC_Init 0 */

  /* USER CODE BEGIN CORDIC_Init 1 */
  CORDIC_ConfigTypeDef hcordicConfig;

	hcordicConfig.Function = CORDIC_FUNCTION_COSINE; /* Computes cos + sin simultaneously */
	hcordicConfig.Precision = CORDIC_PRECISION_6CYCLES; /* 6 iterations = ~20-bit accuracy */
	hcordicConfig.Scale = CORDIC_SCALE_0; /* No scaling needed for sin/cos */
	hcordicConfig.NbWrite = CORDIC_NBWRITE_1; /* 1 input: angle only */
	hcordicConfig.NbRead = CORDIC_NBREAD_1; /* 2 outputs: cos (primary) + sin (secondary) */
	hcordicConfig.InSize = CORDIC_INSIZE_16BITS; /* 32-bit input (we'll convert from 16-bit) */
	hcordicConfig.OutSize = CORDIC_OUTSIZE_16BITS; /* 32-bit output for best precision */

  /* USER CODE END CORDIC_Init 1 */
  hcordic.Instance = CORDIC;
  if (HAL_CORDIC_Init(&hcordic) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CORDIC_Init 2 */
  
  if (HAL_CORDIC_Configure(&hcordic, &hcordicConfig) != HAL_OK)
			{
		Error_Handler();
	}

  /* USER CODE END CORDIC_Init 2 */

}

void HAL_CORDIC_MspInit(CORDIC_HandleTypeDef* cordicHandle)
{

  if(cordicHandle->Instance==CORDIC)
  {
  /* USER CODE BEGIN CORDIC_MspInit 0 */

  /* USER CODE END CORDIC_MspInit 0 */
    /* CORDIC clock enable */
    __HAL_RCC_CORDIC_CLK_ENABLE();
  /* USER CODE BEGIN CORDIC_MspInit 1 */

  /* USER CODE END CORDIC_MspInit 1 */
  }
}

void HAL_CORDIC_MspDeInit(CORDIC_HandleTypeDef* cordicHandle)
{

  if(cordicHandle->Instance==CORDIC)
  {
  /* USER CODE BEGIN CORDIC_MspDeInit 0 */

  /* USER CODE END CORDIC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CORDIC_CLK_DISABLE();
  /* USER CODE BEGIN CORDIC_MspDeInit 1 */

  /* USER CODE END CORDIC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
  * @brief  Compute sine and cosine of an electrical angle using the CORDIC
  *         coprocessor (configured in MX_CORDIC_Init as COSINE, 6-cycle
  *         precision, 1x16-bit write, 1x16-bit+16-bit read).
  * @note   No RRDY polling is performed: reading CORDIC_RDATA before the
  *         result is ready automatically stalls the bus (hardware wait
  *         states) until the computation completes, so the read below
  *         already blocks for exactly as long as necessary.
  * @param  uAngle Angle, scaled over the full uint16_t range (0..65535 => 0..2*pi).
  * @param  pfSinAlpha Output pointer, sine of the angle.
  * @param  pfCosAlpha Output pointer, cosine of the angle.
  */
void CORDIC_ComputeSinCos(uint16_t uAngle, volatile float *pfSinAlpha, volatile float *pfCosAlpha)
{
  uint32_t uRawData;

  hcordic.Instance->WDATA = 0x7FFF0000U | (uint32_t)uAngle;

  uRawData = hcordic.Instance->RDATA;

  *pfCosAlpha = (float)(int16_t)(uRawData) * (1.0f / 32767.0f);
  *pfSinAlpha = (float)(int16_t)(uRawData >> 16) * (1.0f / 32767.0f);
}

/* USER CODE END 1 */
