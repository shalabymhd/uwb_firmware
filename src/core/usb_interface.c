/**
  ******************************************************************************
  * @file    usb_interface.c
  * @brief   This file provides code used to interface with ROS through USB.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_interface.h"
#include "main.h"
#include "stm32f4xx_it.h"
#include <stdlib.h>
#include "common.h"

/* Variables -----------------------------------------------------------------*/
extern uint8_t CdcReceiveBuffer[USB_BUFFER_SIZE];

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: read_usb()
 *
 * The purpose of this function is to read the oldest message at the USB port,
 * and remove the message from the USB-receive buffer.
 * 
 */
void read_usb(){
    char *idx_beg;
    char *idx_end;

    char print_stat[20]; 

    idx_beg = &CdcReceiveBuffer; // address of the USB-receive buffer
    idx_end = strstr(CdcReceiveBuffer, "\n"); // address where to stop reading the message

    uint8_t len = idx_end - idx_beg - 1; // Removing the first entry 
    if (idx_end > 0)
    {
        /* ----------------------- PROCESS COMMUNICATED INFORMATION ----------------------- */
        uint8_t *res = (uint8_t*)malloc(sizeof(uint8_t)*(len+1)); // allocated dynamic memory
        if (res == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

        // TODO: temporary - to be replaced with actual global switch variable
        strncpy(res, idx_beg + 1, len);
        res[len] = '\0';
        sprintf(print_stat, "%s\n", res);
        usb_print(print_stat);

        /* ------------------------------ UPDATE THE BUFFER ------------------------------ */
        res = realloc(res, sizeof(uint8_t)*(USB_BUFFER_SIZE - 1)); // reallocate dynamic memory
        if (res == NULL) {MemManage_Handler();} // if the memory has not been allocated, interrupt operations

        memcpy(res, CdcReceiveBuffer + len + 2, USB_BUFFER_SIZE - len - 2); // copy unread buffer into temp memory
        memset(CdcReceiveBuffer + 1, '\0', USB_BUFFER_SIZE - 1); // clear the buffer
        memcpy(CdcReceiveBuffer + 1, res, USB_BUFFER_SIZE - len - 1); // move data back to buffer
        CdcReceiveBuffer[0] = CdcReceiveBuffer[0] - len - 1; // adjust where to continue writing

        // free the temporary memory 
        free(res);
    }
} // end dw_test()
