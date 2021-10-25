/**
  ******************************************************************************
  * @file    usb.h
  * @brief   This file contains all the function prototypes for
  *          the usb.c file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_H__
#define __SPI_H__

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

#endif /* __SPI_H__ */