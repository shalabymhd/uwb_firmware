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
#include "main.h" // TODO: 
#include <stdbool.h>

/* Variable declarations ------------------------------------------------------------------*/
bool success;
uint8_t target_ID;
struct int_params *i;
uint8_t rec_meas_bool;
struct bool_params *b;


FsmAllStates FSM_status = IDLE; // TODO: Why is this here?

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
        /* Extract the target */
        HASH_FIND_STR(FSM_int_params, "target", i);
        target_ID = i->value;
        free(i);

        /* Extract the toggle that dictates if the receiver computes range measurements */
        HASH_FIND_STR(FSM_bool_params, "rec_meas", b);
        rec_meas_bool = b->value;
        free(b);

        if (target_ID == BOARD_ID){
          usb_print("TWR FAIL: The target ID is the same as the initiator's ID.\r\n");
          break;
        }

        success = twrInitiateInstance(target_ID, rec_meas_bool);

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