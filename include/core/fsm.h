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

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Private typedef -----------------------------------------------------------*/               
extern enum FSM_all_states {
	IDLE,        // 0 = inactive, tag in receive mode
	INFINITE_TWR,// 1 = initiate two-way ranging indefinitely
	INITIATE_TWR,// 2 = initiate an instance two-way ranging
} FSM_status;

/* Function Prototypes -------------------------------------------------------*/
void fsmLoop();

/* Variables -----------------------------------------------------------*/
extern struct int_params *FSM_int_params;
extern struct float_params *FSM_float_params;
extern struct bool_params *FSM_bool_params;
extern struct str_params *FSM_str_params;

#ifdef __cplusplus
}
#endif

#endif /* __FSM__ */