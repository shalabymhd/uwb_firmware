/**
  ******************************************************************************
  * @file    ranging.h
  * @brief   This file contains all the function prototypes for
  *          the ranging.c file
  * 
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RANGING_H__
#define __RANGING_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
// TODO: ideally, many of these should actually be in the ranging.c file
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

/* Typedef -------------------------------------------------------------------*/
typedef signed long long int64;
typedef unsigned long long uint64;

/* Function Prototypes -------------------------------------------------------*/
void ranging_init(void);
void uwbFrameHandler(void);
int twrInitiateInstance(uint8_t, bool, uint8_t);
int twrReceiveCallback(void);
int txTimestampsSS(uint64_t, uint64_t, float*, float*, uint16_t*, float*, bool);
int txTimestampsDS(uint64_t, uint64_t, uint64_t, float*, float*, uint16_t*, float*, bool);
int rxTimestampsSS(uint64_t, uint8_t, float*, float*, uint16_t*, float*, bool);
int rxTimestampsDS(uint64_t, uint64_t, uint8_t, float*, float*, uint16_t*, float*, bool);
int passivelyListenSS(uint32_t, bool);
int passivelyListenDS(uint32_t, bool);
bool checkReceivedFrame(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
void setPassiveToggle(bool);
void setResponseDelay(uint16);

#define UUS_TO_DWT_TIME 65536
#define MAX_FRAME_LEN 100

#ifdef __cplusplus
}
#endif

#endif /* __RANGING_H__ */