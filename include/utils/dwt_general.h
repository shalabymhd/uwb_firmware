/**
  ******************************************************************************
  * @file    dwt_general.h
  * @brief   This file contains all the function prototypes for
  *          the dwt_general.c file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DWT_GENERAL_H__
#define __DWT_GENERAL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "deca_device_api.h"

/* Type defines --------------------------------------------------------------*/


/* Defines -------------------------------------------------------------------*/

/* Variable Declarations -----------------------------------------------------*/

/* Function Prototypes -------------------------------------------------------*/
uint32_t DWT_Delay_Init(void);

/* Returns the interval between previous_ticks and current ticks. */
uint32_t getInterval(uint32_t* previous_ticks_buff);

void uwb_init(void);
void reset_DW1000(void);

typedef unsigned long long uint64;
uint64 get_tx_timestamp_u64(void);
uint64 get_rx_timestamp_u64(void);
void final_msg_set_ts(uint8 *ts_field, uint64 ts);
void final_msg_get_ts(const uint8 *ts_field, uint32 *ts);

#ifdef __cplusplus
}
#endif

#endif /* __DWT_GENERAL_H__ */