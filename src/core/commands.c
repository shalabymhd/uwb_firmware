/**
  ******************************************************************************
  * @file    commands.c
  * @brief   The main file for all the USB command implementations. All commands
  *          functions must have the same inputs and outputs.
  * 
    NOTE: currently ALL commands are endlessly retried until they return a 
    value of 1, or until a new command sent over USB overwrites. This might not 
    be the desired behavior for some future functions, where they might just 
    want to report a failure and not retry. 

    A simple solution is to extend the possible return values of the commands:
    -1: Fail, retry me.
    0: Fail, 
    1: success
    
  ******************************************************************************
  */

#include "commands.h"
#include "common.h"
#include "ranging.h"
#include "uthash.h"
#include "main.h" 
#include <stdbool.h>

int c00_set_inactive(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs){
    usb_print("R00\r");
    return 1;
}

int c01_get_id(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs){
    uint8_t my_id;
    dwt_geteui(&my_id);
    if(my_id != BOARD_ID){
        usb_print("Decawave ID and BOARD_ID do not match.");
        return 0;
    }

    char id_str[5];
    sprintf(id_str, "R01,%i\r", my_id); 
    usb_print(id_str);
    return 1;
}

int c02_initiate_twr(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs){
    bool success;
    uint8_t target_ID;
    IntParams *i;
    bool target_meas_bool;
    BoolParams *b;

    /* Extract the target */
    HASH_FIND_STR(msg_ints, "target", i);
    target_ID = i->value;

    /* Extract the toggle that dictates if the target computes range measurements */
    HASH_FIND_STR(msg_bools, "targ_meas", b);
    target_meas_bool = b->value;

    if (target_ID == BOARD_ID){
        usb_print("TWR FAIL: The target ID is the same as the initiator's ID.\r\n");
        return 0;
    }

    success = twrInitiateInstance(target_ID, target_meas_bool);

    if (success){ 
        usb_print("TWR SUCCESS!\r\n"); // placeholder
        return 1;
    }
    else {
        usb_print("TWR FAIL: No successful response.\r\n");
        return 0;
    }
}
