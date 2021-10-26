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

/* Function Prototypes -------------------------------------------------------*/
int do_owr(void);
void listen(void);
void uwb_init(void);
void reset_DW1000(void);

#define UUS_TO_DWT_TIME 65536
/* Default antenna delay values for 64 MHz PRF. */
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436

#define POLL_RX_TIMEOUT 10000
#define RESP_RX_TIMEOUT 2000 //2700
#define FINAL_RX_TIMEOUT 2000
#define REPORT_RX_TIMEOUT 1000
#define POLL_TX_TO_RESP_RX_DLY_UUS 150

#define PRE_TIMEOUT 8

#ifdef __cplusplus
}
#endif

#endif /* __RANGING_H__ */