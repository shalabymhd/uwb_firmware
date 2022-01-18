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
int twrInitiateInstance(void);
int twrReceiveCallback(void);
void uwbReceiveInterruptInit(void);

#define UUS_TO_DWT_TIME 65536

#ifdef __cplusplus
}
#endif

#endif /* __RANGING_H__ */