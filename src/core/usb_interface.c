/**
  ******************************************************************************
  * @file    usb_interface.c
  * @brief   This file provides code used to interface with python through USB.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_interface.h"
#include "stm32f4xx_it.h"
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include "commands.h"
#include "dwt_iqr.h"
/* Typedefs ------------------------------------------------------------------*/
typedef enum {INT=1, STR=2, BOOL=3, FLOAT=4} FieldTypes;

/* Variables -----------------------------------------------------------------*/
static int command_number = -1;

static const char *c00_fields[1]; // No fields. Empty array of size 1
static const FieldTypes c00_types[1];  // No fields. Empty array of size 1
static const char *c01_fields[1]; // No fields. Empty array of size 1
static const FieldTypes c01_types[1]; // No fields. Empty array of size 1
static const char *c02_fields[1]; // No fields. Empty array of size 1
static const FieldTypes c02_types[1]; // No fields. Empty array of size 1
static const char *c03_fields[1]; // No fields. Empty array of size 1
static const FieldTypes c03_types[1]; // No fields. Empty array of size 1
static const char *c04_fields[] = {"toggle"}; // can't be more than 10 characters
static const FieldTypes c04_types[] = {BOOL}; 
static const char *c05_fields[] = {"target", "targ_meas", "mult_twr"}; // can't be more than 10 characters
static const FieldTypes c05_types[] = {INT, BOOL, INT};

static const char **all_command_fields[] = {
  c00_fields,
  c01_fields,
  c02_fields,
  c03_fields,
  c04_fields,
  c05_fields
  };

static const FieldTypes *all_command_types[] = {
  c00_types,
  c01_types,
  c02_types,
  c03_types,
  c04_types,
  c05_types
  };

static const int (*all_command_funcs[])(IntParams*, FloatParams*, BoolParams*, StrParams*) = {
  c00_set_idle,
  c01_get_id,
  c02_reset,
  c03_do_tests,
  c04_toggle_passive,
  c05_initiate_twr
  };

/* TODO: can below be made local variables if defined in
 parseMessageIntoHashTables and passed to deleteOldParams() ? Or would we lose
 the handle to these variables after the function exits, and they would exist in 
 the hash tables without any way for us to free the memory. 
*/
static IntParams *msg_ints; 
static FloatParams *msg_floats;
static BoolParams *msg_bools;
static StrParams *msg_strs;

/* Private Functions ----------------------------------------------------------*/
static void parseMessageIntoHashTables(char *msg);
static void deleteOldParams();

/*! ----------------------------------------------------------------------------
 * Function: readUsb()
 *
 * The purpose of this function is to read the oldest message at the USB port,
 * and remove the message from the USB-receive buffer.
 * Additionally, this function updates the status and parameters associated with 
 * the finite-state machine.
 * 
 */
void readUsb(){
  
    decaIrqStatus_t stat;
    stat = decamutexon();
    char *idx_end;

    // char print_stat[20];
    
    idx_end = strstr(CdcReceiveBuffer, "\r"); // address where to stop reading the message

    uint8_t len = idx_end - CdcReceiveBuffer - 1; // Removing the first entry 
    if (idx_end > 0)
    {
        /* ----------------------- PROCESS COMMUNICATED INFORMATION ----------------------- */
        char *dyn = (char*)malloc(sizeof(char)*(len+1)); // allocated dynamic memory
        if (dyn == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

        // Extract the oldest unread message.
        strncpy(dyn, CdcReceiveBuffer + 1, len);
        dyn[len] = '\0';

        parseMessageIntoHashTables(dyn); 

        /* ------------------------------ UPDATE THE BUFFER ------------------------------ */
        dyn = realloc(dyn, sizeof(char)*(USB_BUFFER_SIZE - 1)); // reallocate dynamic memory
        if (dyn == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

        memcpy(dyn, CdcReceiveBuffer + len + 2, USB_BUFFER_SIZE - len - 2); // copy unread buffer into temp memory
        memset(CdcReceiveBuffer + 1, '\0', USB_BUFFER_SIZE - 1); // clear the buffer
        memcpy(CdcReceiveBuffer + 1, dyn, USB_BUFFER_SIZE - len - 1); // move data back to buffer
        CdcReceiveBuffer[0] = CdcReceiveBuffer[0] - len - 1; // adjust where to continue writing

        // free the temporary memory 
        free(dyn);
    }
    
    decamutexoff(stat);
    
    /* 
    NOTE: currently ALL commands are endlessly retried until they return a 
    value of 1, or until a new command sent over USB overwrites. This might not 
    be the desired behavior for some future functions, where they might just 
    want to report a failure and not retry. 

    A simple solution is to extend the possible return values of the commands:
    -1: Fail, retry me.
    0: Fail
    1: success
    */

    // Call if a valid command number was detected
    int num_commands = sizeof(all_command_funcs) / sizeof(all_command_funcs[0]);
    if (command_number >= 0 && command_number <= num_commands){
      bool success;
      success = (*all_command_funcs[command_number])(
        msg_ints,
        msg_floats,
        msg_bools,
        msg_strs);

      if (success){
        command_number = -1;
      }
    }
} // end readUsb()


/*! -----------------------------------------------------------------------------------------
 * Function: parseMessageIntoHashTables()
 *
 * The purpose of this function is to update the status of the FSM and extract the 
 * corresponding params.
 * 
 */
void parseMessageIntoHashTables(char *msg){
    char *pt = strtok(msg,","); // break the string using the comma delimiter, to read each entry separately
    int iter = -1;
    const FieldTypes *msg_types; // Pointer to array
    const char **msg_fields; // Pointer to array of pointers
    // TODO: lol indent with 2 spaces or 4 spaces?

    deleteOldParams(); // delete all old params

    while (pt != NULL) { // while there still exists an unread parameter 
        if (iter == -1){
          command_number = atoi(pt+1); // the first entry of "msg" corresponds to the command number
          msg_types = all_command_types[command_number];
          msg_fields = all_command_fields[command_number];
        }
        else{
          FieldTypes type = msg_types[iter]; 
          switch (type)
          {
          case INT:{
            IntParams *param_temp;

            param_temp = malloc(sizeof(IntParams));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, msg_fields[iter]);
            param_temp->value = atoi(pt);
            
            HASH_ADD_STR(msg_ints, key, param_temp);

            break;
          }
          case STR:{
            StrParams *param_temp;

            param_temp = malloc(sizeof(StrParams));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations
            
            strcpy(param_temp->key, msg_fields[iter]); 
            strcpy(param_temp->value, pt);
            
            HASH_ADD_STR(msg_strs, key, param_temp);

            break;
          }
          case BOOL:{
            BoolParams *param_temp;

            param_temp = malloc(sizeof(BoolParams));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations
            
            strcpy(param_temp->key, msg_fields[iter]); 
            param_temp->value = atoi(pt);
            
            HASH_ADD_STR(msg_bools, key, param_temp);

            break;
          }
          case FLOAT:{
            FloatParams *param_temp;
            
            char char_temp[2];
            int int_temp;
            char float_bytes[4];

            /* Hex string to float conversion */ 
            memcpy(char_temp, pt, 2); 
            int_temp = (int)strtol(char_temp, NULL, 16); 
            float_bytes[3] = int_temp;

            memcpy(char_temp, pt+2, 2); 
            int_temp = (int)strtol(char_temp, NULL, 16); 
            float_bytes[2] = int_temp;

            memcpy(char_temp, pt+4, 2); 
            int_temp = (int)strtol(char_temp, NULL, 16);
            float_bytes[1] = int_temp;

            memcpy(char_temp, pt+6, 2); 
            int_temp = (int)strtol(char_temp, NULL, 16); 
            float_bytes[0] = int_temp;

            param_temp = malloc(sizeof(FloatParams));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations
            
            strcpy(param_temp->key, msg_fields[iter]); 
            memcpy(&(param_temp->value), &float_bytes, 4);
            
            HASH_ADD_STR(msg_floats, key, param_temp);
            
            break;
          }
          default:
            break;
          }
        }

        iter += 1;
        pt = strtok(NULL, ","); // gets read of read parameter, sets current parameter to the next one
    }
} // end parseMessageIntoHashTables()


void deleteOldParams() {

  /* Delete int params */
  IntParams *current_int, *tmp_int;

  HASH_ITER(hh, msg_ints, current_int, tmp_int) {
    HASH_DEL(msg_ints, current_int);  /* delete; advances to next param */
    free(current_int);    
  }

  /* Delete str params */
  StrParams *current_str, *tmp_str;

  HASH_ITER(hh, msg_strs, current_str, tmp_str) {
    HASH_DEL(msg_strs, current_str);  /* delete; advances to next param */
    free(current_str);    
  }

  /* Delete bool params */
  BoolParams *current_bool, *tmp_bool;

  HASH_ITER(hh, msg_bools, current_bool, tmp_bool) {
    HASH_DEL(msg_bools, current_bool);  /* delete; advances to next param */
    free(current_bool);    
  }

  /* Delete float params */
  FloatParams *current_float, *tmp_float;

  HASH_ITER(hh, msg_floats, current_float, tmp_float) {
    HASH_DEL(msg_floats, current_float);  /* delete; advances to next param */
    free(current_float);    
  }
}