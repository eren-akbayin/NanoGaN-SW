/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_cdc_acm.c
 * @author  MCD Application Team
 * @brief   USBX Device CDC ACM applicative source file
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
#include "ux_device_cdc_acm.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
UX_SLAVE_CLASS_CDC_ACM *cdc_acm;
uint8_t UserRxBuffer[16];
uint8_t flagTXReq = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  USBD_CDC_ACM_Activate
  *         This function is called when insertion of a CDC ACM device.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Activate */
	cdc_acm = (UX_SLAVE_CLASS_CDC_ACM*)cdc_acm_instance;
  /* USER CODE END USBD_CDC_ACM_Activate */

  return;
}

/**
  * @brief  USBD_CDC_ACM_Deactivate
  *         This function is called when extraction of a CDC ACM device.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Deactivate */
	cdc_acm = UX_NULL;
  /* USER CODE END USBD_CDC_ACM_Deactivate */

  return;
}

/**
  * @brief  USBD_CDC_ACM_ParameterChange
  *         This function is invoked to manage the CDC ACM class requests.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_ParameterChange */
	UX_PARAMETER_NOT_USED(cdc_acm_instance);
  /* USER CODE END USBD_CDC_ACM_ParameterChange */

  return;
}

/* USER CODE BEGIN 2 */
VOID usbx_cdc_read_thread_entry(ULONG thread_input)
{
	/* Local Variables */
	ULONG actual_length;
	UX_SLAVE_DEVICE *device;
	device = &_ux_system_slave->ux_system_slave_device;
	/* Infinite Loop */
	while (1)
	{
		/* Check if device is configured */
		if ((device->ux_slave_device_state == UX_DEVICE_CONFIGURED) && (cdc_acm != UX_NULL))
				{
			ux_device_class_cdc_acm_read(cdc_acm, (UCHAR*)UserRxBuffer, 16, &actual_length);
			for (uint8_t i = 0; i < actual_length; i++)
					{
				switch (UserRxBuffer[i])
				{
				case '1':
					flagTXReq = 1;
					break;
				case '0':
					flagTXReq = 0;
					break;
				}
			}
		}
		tx_thread_sleep(1);
	}
	/* Sleep for 10ms */

}

VOID usbx_cdc_write_thread_entry(ULONG thread_input)
{
	/* Local Variables */
	ULONG actual_length;
	UX_SLAVE_DEVICE *device;
	device = &_ux_system_slave->ux_system_slave_device;
	/* Infinite Loop */
	while (1)
	{
		/* Check if device is configured */
		if ((device->ux_slave_device_state == UX_DEVICE_CONFIGURED) && (cdc_acm != UX_NULL) && flagTXReq == 1)
				{
			ux_device_class_cdc_acm_write(cdc_acm, (UCHAR*)&gInverterMeasurements, sizeof(gInverterMeasurements),
					&actual_length);
			flagTXReq = 0;
			/* Sleep for 1s */
			//tx_thread_sleep(1);
		}
	}
}

/* USER CODE END 2 */
