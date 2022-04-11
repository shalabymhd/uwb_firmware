/**
  ******************************************************************************
  * @file    commands.h
  * @brief   Header file for all the USB command implementations.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "uthash.h"
#include "main.h"
/* Typedefs -----------------------------------------------------------*/   

// hash table implementation to store integer parameters
typedef struct IntParams {
    char key[10];      /* field used as the key */
    int value;     /* field used to store integers */
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
    char value[200];      /* field used to store string messages */
    UT_hash_handle hh; /* makes this structure hashable */
}StrParams;     

// hash table implementation to store string parameters
typedef struct ByteParams {
    char key[10];      /* field used as the key */
    uint8_t value[USB_BUFFER_SIZE];      /* pointer to first element of byte array */
    uint16_t len;        /* length of the byte array */
    UT_hash_handle hh; /* makes this structure hashable */
}ByteParams; 

/* Function Prototypes -------------------------------------------------------*/
int c00_set_idle(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);
int c01_get_id(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);
int c02_reset(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);
int c03_do_tests(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);
int c04_toggle_passive(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);
int c05_initiate_twr(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);
int c06_broadcast(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);
int c07_get_max_frame_len(IntParams*, FloatParams*, BoolParams*, StrParams*, ByteParams*);


/* Variables -----------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __COMMANDS_H__ */