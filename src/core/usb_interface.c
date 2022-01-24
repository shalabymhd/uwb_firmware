/**
  ******************************************************************************
  * @file    usb_interface.c
  * @brief   This file provides code used to interface with ROS through USB.
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

/* Variables -----------------------------------------------------------------*/
extern char CdcReceiveBuffer[USB_BUFFER_SIZE];
extern uint8_t FSM_status;
extern struct int_params *FSM_int_params;
extern struct float_params *FSM_float_params;
extern struct bool_params *FSM_bool_params;
extern struct str_params *FSM_str_params;


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: readUsb()
 *
 * The purpose of this function is to read the oldest message at the USB port,
 * and remove the message from the USB-receive buffer.
 * Additionally, this function updates the status and parameters associated with 
 * the finite-state machine.
 * 
 */
void readUsb(){
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

        updateCommandsAndParams(dyn); 

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
} // end readUsb()


/*! -----------------------------------------------------------------------------------------
 * Function: updateCommandsAndParams()
 *
 * The purpose of this function is to update the status of the FSM and extract the 
 * corresponding params.
 * 
 */
void updateCommandsAndParams(char *msg){
    char *pt = strtok(msg,","); // break the string using the comma delimiter, to read each entry separately
    int iter = -1;
          
    deleteOldParams(); // delete all old params

    while (pt != NULL) { // while there still exists an unread parameter 
        if (iter == -1){
          FSM_status = atoi(pt+1); // the first entry of "msg" corresponds to the command number
        }
        else{
          int type = atoi(CO2_types[iter]); // TODO: generalize
          switch (type)
          {
          case 1:{
            struct int_params *param_temp;

            param_temp = malloc(sizeof(struct int_params));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

            strcpy(param_temp->key, CO2_fields[iter]); // TODO: generalize
            param_temp->value = atoi(pt);
            
            HASH_ADD_STR(FSM_int_params, key, param_temp);

            break;
          }
          case 2:{
            struct str_params *param_temp;

            param_temp = malloc(sizeof(struct str_params));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations
            
            strcpy(param_temp->key, CO2_fields[iter]); // TODO: generalize
            strcpy(param_temp->value, pt);
            
            HASH_ADD_STR(FSM_str_params, key, param_temp);

            break;
          }
          case 3:{
            struct bool_params *param_temp;

            param_temp = malloc(sizeof(struct bool_params));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations
            
            strcpy(param_temp->key, CO2_fields[iter]); // TODO: generalize
            param_temp->value = pt;
            
            HASH_ADD_STR(FSM_bool_params, key, param_temp);

            break;
          }
          case 4:{
            struct float_params *param_temp;
            
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

            param_temp = malloc(sizeof(struct float_params));
            if (param_temp == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations
            
            strcpy(param_temp->key, CO2_fields[iter]); // TODO: generalize
            memcpy(&(param_temp->value), &float_bytes, 4);
            
            HASH_ADD_STR(FSM_float_params, key, param_temp);
            
            break;
          }
          default:
            break;
          }
        }

        iter += 1;
        pt = strtok(NULL, ","); // gets read of read parameter, sets current parameter to the next one
    }
} // end updateCommandsAndParams()


void deleteOldParams() {

  /* Delete int params */
  struct int_params *current_int, *tmp_int;

  HASH_ITER(hh, FSM_int_params, current_int, tmp_int) {
    HASH_DEL(FSM_int_params, current_int);  /* delete; advances to next param */
    free(current_int);    
  }

  /* Delete str params */
  struct str_params *current_str, *tmp_str;

  HASH_ITER(hh, FSM_str_params, current_str, tmp_str) {
    HASH_DEL(FSM_str_params, current_str);  /* delete; advances to next param */
    free(current_str);    
  }

  /* Delete bool params */
  struct bool_params *current_bool, *tmp_bool;

  HASH_ITER(hh, FSM_bool_params, current_bool, tmp_bool) {
    HASH_DEL(FSM_bool_params, current_bool);  /* delete; advances to next param */
    free(current_bool);    
  }

  /* Delete float params */
  struct float_params *current_float, *tmp_float;

  HASH_ITER(hh, FSM_float_params, current_float, tmp_float) {
    HASH_DEL(FSM_float_params, current_float);  /* delete; advances to next param */
    free(current_float);    
  }
}