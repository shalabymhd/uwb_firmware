/**
  ******************************************************************************
  * @file    usb.h
  * @brief   This file contains all the function prototypes for
  *          the usb.c file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_H__
#define __USB_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"

/* Function Prototypes -------------------------------------------------------*/
void usb_print(char*);

#ifdef __cplusplus
}
#endif

#endif /* __USB_H__ */