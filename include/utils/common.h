/**
  ******************************************************************************
  * @file    common.h
  * @brief   This file contains all the function prototypes for
  *          the common.c file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __COMMON_H__
#define __COMMON_H__


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"
#include "stm32f4xx_hal.h"

typedef struct{
	float x;
	float y;
	float z;
} element_R3;

void usb_print(char*);
void convert_float_to_string(char* stringbuff,float data);
void convert_elementR3_to_string(char* str, element_R3 data);

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_H__ */
