/**
  ******************************************************************************
  * @file    fsm.h
  * @brief   This file contains all the function prototypes for
  *          the fsm.c file
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FSM_H__
#define __FSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
void fsmLoop();

               
extern enum FSM_status {
	IDLE,        // 0 = inactive, tag in receive mode
	INFINITE_TWR,// 1 = initiate two-way ranging indefinitely
	INITIATE_TWR,// 2 = initiate an instance two-way ranging
} FSM_status;



#ifdef __cplusplus
}
#endif

#endif /* __FSM__ */