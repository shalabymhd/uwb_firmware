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
    char *pt = strtok (msg,",");
    int iter = -1;
    while (pt != NULL) {
        // char a = atoi(pt);
        // printf("%d\n", a);
        if (iter == -1){
          FSM_status = atoi(pt+1);
        }
        else{
          int type = atoi(CO2_types[iter]);
          switch (type)
          {
          case 1:{
            struct int_params *param_temp;

            param_temp = malloc(sizeof(struct int_params));
            strcpy(param_temp->key, CO2_fields[iter]);
            param_temp->value = atoi(pt);
            HASH_ADD_STR(FSM_int_params, key, param_temp);
            break;
          }
          case 2:{
            struct str_params *param_temp;

            param_temp = malloc(sizeof(struct str_params));
            strcpy(param_temp->key, CO2_fields[iter]);
            strcpy(param_temp->value, pt);
            HASH_ADD_STR(FSM_str_params, key, param_temp);
            break;
          }
          case 3:{
            struct bool_params *param_temp;

            param_temp = malloc(sizeof(struct bool_params));
            strcpy(param_temp->key, CO2_fields[iter]);
            param_temp->value = pt;
            HASH_ADD_STR(FSM_bool_params, key, param_temp);
            break;
          }
          case 4:{
            struct float_params *param_temp;
            char char_temp[2];
            int int_temp;
            char float_bytes[4];

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
            strcpy(param_temp->key, CO2_fields[iter]);
            memcpy(&(param_temp->value), &float_bytes, 4);
            HASH_ADD_STR(FSM_float_params, key, param_temp);
            break;
          }
          default:
            break;
          }
        }

        iter += 1;
        pt = strtok(NULL, ",");
    }

    struct float_params *s = NULL;
    HASH_FIND_STR(FSM_float_params, "timestamp", s);

    usb_print(s->key);

} // end updateCommandsAndParams()