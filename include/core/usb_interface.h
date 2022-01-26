/**
  ******************************************************************************
  * @file    usb_interface.h
  * @brief   This file contains all the function prototypes for
  *          the usb_interface.c file
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_INTERFACE_H__
#define __USB_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Function Prototypes -------------------------------------------------------*/
void readUsb();
void updateCommandsAndParams(char *msg);
void deleteOldParams();

/* Variables -----------------------------------------------------------*/
static const char *CO2_fields[] = {"target", "rec_meas"}; // can't be more than 10 characters
static const char *CO2_types[] = {"1", "3"}; // 1=int, 2=str, 3=bool, 4=float

#ifdef __cplusplus
}
#endif

#endif /* __USB_INTERFACE_H__ */