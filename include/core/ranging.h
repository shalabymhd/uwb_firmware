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

/* Typedef -------------------------------------------------------------------*/
typedef signed long long int64;
typedef unsigned long long uint64;

/* Function Prototypes -------------------------------------------------------*/
int twrInitiateInstance(uint8_t, bool, uint8_t);
int twrReceiveCallback(void);
void uwbReceiveInterruptInit(void);
int txTimestampsSS(uint64_t, uint64_t, bool);
int txTimestampsDS(uint64_t, uint64_t, uint64_t, bool);
int rxTimestampsSS(uint64_t, uint8_t, bool);
int rxTimestampsDS(uint64_t, uint64_t, uint8_t, bool);

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: passivelyListen()
 *
 * @brief Listen passively to other TWR tags and record all timestamps.
 * 
 * @param rx_ts1 (uint32_t) The time of reception of the already detected signal through interrupt.
 * @param four_signals (bool) Whether or not a fourth signal is expected.
 * 
 * @return (bool) Success boolean.
 */
int passivelyListen(uint32_t, bool);


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: timestampReceivedFrame()
 *
 * @brief This function retrieves the reception time-stamp of a signal. 
 *
 * NOTE: This function can be called to retrieve the reception time-stamp from the registers or from the embedded
 *       message in a final signal.
 * 
 * @param ts (uint32_t*) Pointer to where to store the time-stamp.
 * @param master_idx (uint8_t) The index of the master ID in the received message.
 * @param master_id (uint8_t) The ID of the master board in this TWR transaction. 
 * @param slave_idx (uint8_t) The index of the slave ID in the received message.
 * @param slave_id (uint8_t) The ID of the slave board in this TWR transaction.
 * @param final_signal (bool) Toggle between reading reception time-stamp from the register (0) vs. the embedded message (1).
 * 
 * @return (bool) Success boolean.
 */
bool timestampReceivedFrame(uint32_t*, uint8_t, uint8_t, uint8_t, uint8_t, bool);


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: setPassiveToggle()
 * 
 * @brief This function sets the passive toggle.
 * 
 * @param toggle (bool) 1 if passive toggle is to be turned on, 0 otherwise.
 */
void setPassiveToggle(bool);

#define UUS_TO_DWT_TIME 65536

#ifdef __cplusplus
}
#endif

#endif /* __RANGING_H__ */