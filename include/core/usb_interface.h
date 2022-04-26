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
#include "cmsis_os.h"
/* Private typedef -----------------------------------------------------------*/

/* Function Prototypes -------------------------------------------------------*/
void readUsb();
void interfaceInit(void);
osMailQId getMailQId(void);

/* Variables -----------------------------------------------------------*/
typedef struct {
    uint8_t msg[USB_MSG_BUFFER_SIZE];
    uint32_t len;
} UsbMsg;


#ifdef __cplusplus
}
#endif

#endif /* __USB_INTERFACE_H__ */