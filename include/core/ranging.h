/**
  ******************************************************************************
  * @file    ranging.h
  * @brief   This file contains all the function prototypes for
  *          the ranging.c file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RANGING_H__
#define __RANGING_H__

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

/* Function Prototypes -------------------------------------------------------*/
int twrInitiateInstance(uint8_t target_ID);
int twrReceiveCallback(void);
void uwbReceiveInterruptInit(void);

#define UUS_TO_DWT_TIME 65536

#ifdef __cplusplus
}
#endif

#endif /* __RANGING_H__ */