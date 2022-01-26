/**
  ******************************************************************************
  * @file    fsm.c
  * @brief   The main file for the finite state machine.
  ******************************************************************************
  */

#include "fsm.h"
#include "common.h"
#include "ranging.h"
#include "uthash.h"
#include "main.h" // TODO: 
#include <stdbool.h>
uint8_t target_ID;
bool success;
struct int_params *s;

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
        HASH_FIND_STR(FSM_int_params, "target", s);
        target_ID = s->value;
        
        if (target_ID == BOARD_ID){
          usb_print("TWR FAIL: The target ID is the same as the initiator's ID.\r\n");
          break;
        }

        success = twrInitiateInstance(target_ID);

        if (success){ 
          usb_print("TWR SUCCESS!\r\n"); // placeholder
          FSM_status = IDLE;
        }
        else {
          usb_print("TWR FAIL: No successful response.\r\n");
        }
        break;
      }
      default:
        break;
      
  }

}