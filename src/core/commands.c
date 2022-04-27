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
#include "messaging.h"
#include "common.h"
#include "usbd_cdc_if.h"
#include <stdlib.h>

int c00_set_idle(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    usb_print("R00\r\n");
    osDelay(1);
    return 1;
}

int c01_get_id(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    char id_str[10];
    sprintf(id_str, "R01|%u\r\n", BOARD_ID); 
    usb_print(id_str);
    osDelay(1);
    return 1;
}

int c02_reset(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    // Reset the UWB receiver. TODO: would we need to reset other things? Could even do a hard reset.
    dwt_rxreset();

    // Turn on the UWB receiver
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    
    usb_print("R02\r\n");
    return 1;
}

int c03_do_tests(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    /* TODO: 
    1) Add any other tests necessary.
    2) When we add tests, it might be worthwhile to give different 
       errors an ID and just output the error ID.

    */
    IntParams *i;
    BoolParams *b; 
    StrParams *s;
    FloatParams *f;
    ByteParams *y;
    char float_as_string[10] = {0};
    char response[300] = {0};

    HASH_FIND_STR(msg_ints, "test_int", i);
    HASH_FIND_STR(msg_strs, "test_str", s);
    HASH_FIND_STR(msg_bools, "test_bool", b);
    HASH_FIND_STR(msg_floats, "test_flt", f);
    HASH_FIND_STR(msg_bytes, "test_byte", y);

    uint8_t my_id;
    uint8_t error_id = 0;
    dwt_geteui(&my_id);
    if(my_id != BOARD_ID){
        error_id = 1;
    }
    convert_float_to_string(float_as_string, f->value);

    sprintf(response,"R03|%u|%d|%s|%d|%s|", error_id, i->value, s->value, b->value, float_as_string);
    uint16_t len = strlen(response);

    memcpy(&response[len], &(y->len), 2);
    len += 2;
    memcpy(&response[len], y->value, y->len);
    len += y->len;
    memcpy(&response[len],"\r\n", 2);
    len += 2;
    CDC_Transmit_FS((u_int8_t *) &response[0], len);
   
    return 1;
}

int c04_toggle_passive(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    BoolParams *i;
    
    /* Extract the toggle */
    HASH_FIND_STR(msg_bools, "toggle", i);
    
    setPassiveToggle(i->value);

    usb_print("R04\r\n");
    
    return 1;
}

int c05_initiate_twr(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    bool success;
    uint8_t target_ID, mult_twr;
    IntParams *i;
    bool target_meas_bool;
    BoolParams *b;

    /* Extract the target */
    HASH_FIND_STR(msg_ints, "target", i);
    target_ID = i->value;

    /* Extract the toggle that dictates if the target computes range measurements */
    HASH_FIND_STR(msg_bools, "targ_meas", b);
    target_meas_bool = b->value;

    /* Extract the toggle that dictates if the multiplicative TWR will be used */
    HASH_FIND_STR(msg_ints, "mult_twr", i);
    mult_twr = i->value;

    if (target_ID == BOARD_ID){
        usb_print("TWR FAIL: The target ID is the same as the initiator's ID.\r\n");
        return 0;
        // TODO: we should not retry!! we will get stuck.
    }

    success = twrInitiateInstance(target_ID, target_meas_bool, mult_twr);

    if (success){ 
        // Response is done inside `twrInitiateInstance`
        usb_print("TWR SUCCESS!\r\n");
        return 1;
    }
    else {
        // TODO: this is worth retrying. need to implement a limit.
        usb_print("TWR FAIL: No successful response.\r\n");
        return 0;
    }
}


int c06_broadcast(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    
    ByteParams *b;
    
    HASH_FIND_STR(msg_bytes, "data", b);
    uint8_t *msg = &(b->value[0]);

    // TODO: we need to standardize the response when success/fail.
    bool success;
    success = broadcast(msg, b->len);
    osDelay(1);
    if (success){
        usb_print("R06\r\n");
        return 1;
    }
    else {
        return 0;
    }
    osDelay(1);
}

int c07_get_max_frame_len(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    char response[20];
    sprintf(response, "R07|%u\r\n", MAX_FRAME_LEN); 
    usb_print(response);
    return 1;
}