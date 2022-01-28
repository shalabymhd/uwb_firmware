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
/* Variables -----------------------------------------------------------*/
// TODO: this variable seems to be declared here, defined in freertos.c, 
// and an identically named variable exists in usbd_cdc_if.c
// something tells me this isnt best practice.
extern char CdcReceiveBuffer[USB_BUFFER_SIZE]; 
#ifdef __cplusplus
}
#endif

#endif /* __USB_INTERFACE_H__ */