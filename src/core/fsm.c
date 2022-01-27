/**
  ******************************************************************************
  * @file    fsm.c
  * @brief   The main file for the finite state machine.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "fsm.h"
#include "common.h"
#include "ranging.h"
#include "uthash.h"
#include "main.h" 
#include <stdbool.h>


FsmAllStates FSM_status = IDLE; // TODO: Why is this here?

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

        char id_str[34];
        sprintf(id_str, "Extended Unique Identifier: %i \n", my_id); // TODO: Standardize the response.
        usb_print(id_str);
        FSM_status = IDLE;
        break;
      }
      case INITIATE_TWR:
      {
        bool success;
        uint8_t target_ID;
        struct int_params *i;
        bool target_meas_bool;
        struct bool_params *b;

        /* Extract the target */
        HASH_FIND_STR(FSM_int_params, "target", i);
        target_ID = i->value;
        free(i);

        /* Extract the toggle that dictates if the target computes range measurements */
        HASH_FIND_STR(FSM_bool_params, "targ_meas", b);
        target_meas_bool = b->value;
        free(b);

        if (target_ID == BOARD_ID){
          usb_print("TWR FAIL: The target ID is the same as the initiator's ID.\r\n");
          break;
        }

        success = twrInitiateInstance(target_ID, target_meas_bool);

        if (success){ 
          usb_print("TWR SUCCESS!\r\n"); // placeholder
        }
        else {
          usb_print("TWR FAIL: No successful response.\r\n");
        }
        FSM_status = IDLE;
        break;
      }
      default:
        break;
      
  }

}