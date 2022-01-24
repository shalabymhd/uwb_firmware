/**
  ******************************************************************************
  * @file    fsm.c
  * @brief   The main file for the finite state machine.
  ******************************************************************************
  */

#include "fsm.h"
#include "common.h"
#include "ranging.h"
#include <stdbool.h>

void fsmLoop(){
  switch (FSM_status)
  {
      case IDLE:
      {
        /* code */
        break;
      }
      case INFINITE_TWR:
      {
        /* code */
        break;
      }
      case INITIATE_TWR:
      {
        bool success;
        // usb_print("Status set to RANGING!\r\n"); // placeholder
        success = twrInitiateInstance();

        if (success){ 
            usb_print("TWR SUCCESS!\r\n"); // placeholder
            FSM_status = 0;
        }
        else {
            // usb_print("TWR FAIL!\r\n");
        }
        break;
      }
      default:
        break;
      
  }

}