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
void uwbFrameHandler(void);
void uwbReceiveInterruptInit(void);
int twrInitiateInstance(uint8_t, bool, uint8_t);
int twrReceiveCallback(void);
int txTimestampsSS(uint64_t, uint64_t, float*, bool);
int txTimestampsDS(uint64_t, uint64_t, uint64_t, float*, bool);
int rxTimestampsSS(uint64_t, uint8_t, float*, bool);
int rxTimestampsDS(uint64_t, uint64_t, uint8_t, float*, bool);

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: passivelyListenSS()
 *
 * @brief Listen passively to other TWR tags and record all timestamps. Single-sided version.
 * 
 * @param rx_ts1 (uint32_t) The time of reception of the already detected signal through interrupt.
 * @param target_meas_bool (bool) Whether or not a third signal is expected.
 * 
 * @return (bool) Success boolean.
 */
int passivelyListenSS(uint32_t, bool);

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: passivelyListenDS()
 *
 * @brief Listen passively to other TWR tags and record all timestamps. Double-sided version.
 * 
 * @param rx_ts1 (uint32_t) The time of reception of the already detected signal through interrupt.
 * @param target_meas_bool (bool) Whether or not a fourth signal is expected.
 * 
 * @return (bool) Success boolean.
 */
int passivelyListenDS(uint32_t, bool);

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: checkReceivedFrame()
 *
 * @brief This function checks if the received frame is the expected one. 
 * 
 * @param initator_idx (uint8_t) The index of the initator ID in the received message.
 * @param initator_id (uint8_t) The ID of the initator board in this TWR transaction. 
 * @param target_idx (uint8_t) The index of the target ID in the received message.
 * @param target_id (uint8_t) The ID of the target board in this TWR transaction.
 * @param msg_type (uint8_t) The type of message expected.
 * 
 * @return (bool) Success boolean.
 */
bool checkReceivedFrame(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: setPassiveToggle()
 * 
 * @brief This function sets the passive toggle.
 * 
 * @param toggle (bool) 1 if passive toggle is to be turned on, 0 otherwise.
 */
void setPassiveToggle(bool);

#define UUS_TO_DWT_TIME 65536
#define MAX_FRAME_LEN 100

#ifdef __cplusplus
}
#endif

#endif /* __RANGING_H__ */