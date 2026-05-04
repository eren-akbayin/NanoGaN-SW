/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_mouse.c
 * @author  MCD Application Team
 * @brief   USBX Device HID Mouse applicative source file
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
#include "ux_device_mouse.h"

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
UX_SLAVE_CLASS_HID *hid_mouse;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  USBD_HID_Mouse_Activate
 *         This function is called when insertion of a HID Mouse device.
 * @param  hid_instance: Pointer to the hid class instance.
 * @retval none
 */
VOID USBD_HID_Mouse_Activate(VOID *hid_instance)
{
	/* USER CODE BEGIN USBD_HID_Mouse_Activate */
	hid_mouse = (UX_SLAVE_CLASS_HID*)hid_instance;
	/* USER CODE END USBD_HID_Mouse_Activate */

	return;
}

/**
 * @brief  USBD_HID_Mouse_Deactivate
 *         This function is called when extraction of a HID Mouse device.
 * @param  hid_instance: Pointer to the hid class instance.
 * @retval none
 */
VOID USBD_HID_Mouse_Deactivate(VOID *hid_instance)
{
	/* USER CODE BEGIN USBD_HID_Mouse_Deactivate */
	hid_mouse = UX_NULL;
	/* USER CODE END USBD_HID_Mouse_Deactivate */

	return;
}

/**
 * @brief  USBD_HID_Mouse_SetReport
 *         This function is invoked when the host sends a HID SET_REPORT
 *         to the application over Endpoint 0.
 * @param  hid_instance: Pointer to the hid class instance.
 * @param  hid_event: Pointer to structure of the hid event.
 * @retval status
 */
UINT USBD_HID_Mouse_SetReport(UX_SLAVE_CLASS_HID *hid_instance,
		UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
	UINT status = UX_SUCCESS;

	/* USER CODE BEGIN USBD_HID_Mouse_SetReport */
	UX_PARAMETER_NOT_USED(hid_instance);
	UX_PARAMETER_NOT_USED(hid_event);
	/* USER CODE END USBD_HID_Mouse_SetReport */

	return status;
}

/**
 * @brief  USBD_HID_Mouse_GetReport
 *         This function is invoked when host is requesting event through
 *         control GET_REPORT request.
 * @param  hid_instance: Pointer to the hid class instance.
 * @param  hid_event: Pointer to structure of the hid event.
 * @retval status
 */
UINT USBD_HID_Mouse_GetReport(UX_SLAVE_CLASS_HID *hid_instance,
		UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
	UINT status = UX_SUCCESS;

	/* USER CODE BEGIN USBD_HID_Mouse_GetReport */
	UX_PARAMETER_NOT_USED(hid_instance);
	UX_PARAMETER_NOT_USED(hid_event);
	/* USER CODE END USBD_HID_Mouse_GetReport */

	return status;
}

/* USER CODE BEGIN 2 */
/* USER CODE BEGIN 1 */
VOID usbx_hid_mouse_thread_entry(ULONG thread_input)
{
	/* Local Variables */
	UX_SLAVE_DEVICE *device;
	UX_SLAVE_CLASS_HID_EVENT hid_event;
	device = &_ux_system_slave->ux_system_slave_device;
	/* Infinite Loop */
	while (1)
	{
		/* Check if the Device is configured */
		if ((device->ux_slave_device_state == UX_DEVICE_CONFIGURED) && (hid_mouse != UX_NULL))
				{
			/* Check if the User Button is pressed */
			if (HAL_GPIO_ReadPin(USR_BTN1_GPIO_Port, USR_BTN1_Pin) == GPIO_PIN_RESET)
					{
				/* Mouse event. Length is fixed to 4 */
				hid_event.ux_device_class_hid_event_length = 4;

				/* Set select position */
				hid_event.ux_device_class_hid_event_buffer[0] = 0;
				/* Set X position */
				hid_event.ux_device_class_hid_event_buffer[1] = 5;

				/* Set Y position */
				hid_event.ux_device_class_hid_event_buffer[2] = 5;

				/* Set wheel position */
				hid_event.ux_device_class_hid_event_buffer[3] = 0;
				/* Send an event to the hid */
				ux_device_class_hid_event_set(hid_mouse, &hid_event);
			}
			else if (HAL_GPIO_ReadPin(USR_BTN2_GPIO_Port, USR_BTN2_Pin) == GPIO_PIN_RESET)
			{
				/* Mouse event. Length is fixed to 4 */
				hid_event.ux_device_class_hid_event_length = 4;

				/* Set select position */
				hid_event.ux_device_class_hid_event_buffer[0] = 0;
				/* Set X position */
				hid_event.ux_device_class_hid_event_buffer[1] = -5;

				/* Set Y position */
				hid_event.ux_device_class_hid_event_buffer[2] = -5;

				/* Set wheel position */
				hid_event.ux_device_class_hid_event_buffer[3] = 0;
				/* Send an event to the hid */
				ux_device_class_hid_event_set(hid_mouse, &hid_event);
			}
		}
		/* Wait for 100ms */
		tx_thread_sleep(10);
	}
}
/* USER CODE END 1 */

/* USER CODE END 2 */
