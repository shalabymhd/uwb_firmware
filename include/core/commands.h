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
#include <stdbool.h>
#include "uthash.h"

/* Typedefs -----------------------------------------------------------*/   

// hash table implementation to store integer parameters
typedef struct IntParams {
    char key[10];      /* field used as the key */
    uint8_t value;     /* field used to store integers */
    UT_hash_handle hh; /* makes this structure hashable */
} IntParams;

// hash table implementation to store float parameters
typedef struct FloatParams {
    char key[10];      /* field used as the key */
    float value;       /* field used to store integers */
    UT_hash_handle hh; /* makes this structure hashable */
} FloatParams;

// hash table implementation to store boolean parameters
typedef struct BoolParams {
    char key[10];      /* field used as the key */
    bool value;     /* field used to store booleans*/
    UT_hash_handle hh; /* makes this structure hashable */
} BoolParams;

// hash table implementation to store string parameters
typedef struct StrParams {
    char key[10];      /* field used as the key */
    char value[20];      /* field used to store string messages */
    UT_hash_handle hh; /* makes this structure hashable */
}StrParams;     

/* Function Prototypes -------------------------------------------------------*/
int c00_set_inactive(IntParams*, FloatParams*, BoolParams*, StrParams*);
int c01_get_id(IntParams*, FloatParams*, BoolParams*, StrParams*);
int c02_initiate_twr(IntParams*, FloatParams*, BoolParams*, StrParams*);

/* Variables -----------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __FSM__ */