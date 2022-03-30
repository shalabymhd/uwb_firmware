/**
  ******************************************************************************
  * @file    messaging.h
  * @brief   This file contains all the function prototypes for
  *          the messaging.h file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MESSAGING_H__
#define __MESSAGING_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "deca_types.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "spi.h"
#include "stm32f4xx_hal_conf.h"
#include "main.h"
#include "common.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "common.h"
#include "dwt_general.h"
#include "dwt_iqr.h"

#ifdef __cplusplus
}
#endif

int broadcast(uint8*, size_t);
int dataReceiveCallback(uint8*);


#endif /* __MESSAGING_H__ */