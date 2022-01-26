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
#include "main.h" 
#include <stdbool.h>

FsmAllStates FSM_status = IDLE;

void fsmLoop(){

  switch (FSM_status)
  {
      case IDLE:
      {
        break;
      }
      case GET_ID:
      { 
        uint8_t my_id;
        dwt_geteui(&my_id);
        if(my_id != BOARD_ID){
          usb_print("Decawave ID and BOARD_ID do not match.");
        }

        char id_str[3];
        sprintf(id_str, "%i \n", my_id); // TODO: Standardize the response.
        usb_print(id_str);
        FSM_status = IDLE;
        break;
      }
      case INITIATE_TWR:
      {
        struct int_params *s;
        uint8_t target_ID;
        bool success;
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