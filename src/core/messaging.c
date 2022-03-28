/**
  ******************************************************************************
  * @file    messaging.c
  * @brief   This file provides functionality and utils for generic messaging
  * between tags.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "messaging.h"

/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 8

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 msg[] = {0x41, 0x88,0,0,'O','U','R',' ','M','O','D','U','L','E',' ', BOARD_ID, 0, 0};

int broadcast(){
    decaIrqStatus_t stat;

    stat = decamutexon();
    dwt_forcetrxoff();

	  /* Set expected response's delay and timeout.
     * As this example only handles one incoming frame with always the same delay and
       timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(40); //dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(800); // dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT*100);

    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
    dwt_writetxdata(sizeof(msg), msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(msg), 0, 1); /* Zero offset in TX buffer, ranging. */

    /* Start transmission, indicating that a response is expected so that reception is 
        enabled automatically after the frame is sent and the delay set by 
        dwt_setrxaftertxdelay() has elapsed. */
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);


    dwt_setpreambledetecttimeout(0);
    dwt_setrxtimeout(0);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    decamutexoff(stat);
    return 1;
}

int dataReceiveCallback(){
  return 0;
}