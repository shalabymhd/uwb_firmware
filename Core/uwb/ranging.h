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
void do_owr(void);
void uwb_init(void);
void reset_DW1000(void);

#ifdef __cplusplus
}
#endif

#endif /* __RANGING_H__ */