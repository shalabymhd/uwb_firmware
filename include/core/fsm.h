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
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/   
      
typedef enum {
	IDLE = 0,        // 0 = inactive, tag in receive mode
	INFINITE_TWR,// 1 = initiate two-way ranging indefinitely
	INITIATE_TWR,// 2 = initiate an instance two-way ranging
} FsmAllStates; 

/* Function Prototypes -------------------------------------------------------*/
void fsmLoop();

/* Variables -----------------------------------------------------------*/
extern FsmAllStates FSM_status;
extern struct int_params *FSM_int_params;
extern struct float_params *FSM_float_params;
extern struct bool_params *FSM_bool_params;
extern struct str_params *FSM_str_params;

#ifdef __cplusplus
}
#endif

#endif /* __FSM__ */