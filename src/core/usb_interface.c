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

/* Variables -----------------------------------------------------------------*/
extern char CdcReceiveBuffer[USB_BUFFER_SIZE];
extern uint8_t FSM_status;
extern struct int_params FSM_int_params;
extern struct float_params FSM_float_params;
extern struct bool_params FSM_bool_params;
extern struct str_params FSM_str_params;

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
        char *res = (char*)malloc(sizeof(char)*(len+1)); // allocated dynamic memory
        if (res == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

        // Extract the oldest unread message.
        strncpy(res, CdcReceiveBuffer + 1, len);
        res[len] = '\0';

        updateCommandsAndParams(res); 

        // TODO: temporary - to be replaced with actual global switch variable
        // strncpy(res, CdcReceiveBuffer + 1, len);
        // res[len] = '\0';
        // sprintf(print_stat, "%s ", res);
        // usb_print(print_stat);

        /* ------------------------------ UPDATE THE BUFFER ------------------------------ */
        res = realloc(res, sizeof(char)*(USB_BUFFER_SIZE - 1)); // reallocate dynamic memory
        if (res == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

        memcpy(res, CdcReceiveBuffer + len + 2, USB_BUFFER_SIZE - len - 2); // copy unread buffer into temp memory
        memset(CdcReceiveBuffer + 1, '\0', USB_BUFFER_SIZE - 1); // clear the buffer
        memcpy(CdcReceiveBuffer + 1, res, USB_BUFFER_SIZE - len - 1); // move data back to buffer
        CdcReceiveBuffer[0] = CdcReceiveBuffer[0] - len - 1; // adjust where to continue writing

        // free the temporary memory 
        free(res);
    }
} // end readUsb()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: updateCommandsAndParams()
 *
 * The purpose of this function is to read the oldest message at the USB port,
 * and remove the message from the USB-receive buffer.
 * 
 */
void updateCommandsAndParams(char *msg){
    char command_str[3] = "C02";
    // strncpy(command_str, msg, 3);
    if (!strncmp(msg, "C02", 3)){
      FSM_status = 2;
    }

} // end updateCommandsAndParams()
