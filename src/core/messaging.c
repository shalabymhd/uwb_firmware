/**
  ******************************************************************************
  * @file    messaging.c
  * @brief   This file provides functionality and utils for generic messaging
  * between tags.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "messaging.h"
#include "ranging.h"

/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 8

/* Message prefix and suffix */
#define PREFIX_LEN 4
#define SUFFIX_LEN 2
uint8 msg_prefix[] = {0x41, 0x88, 0xD, 0};
uint8 msg_suffix[] = {0, 0};

int broadcast(uint8* msg, size_t msg_len){
    // TODO: add check on total message length here.

    // Concatenate arrays into one large array (memory duplication here)
    uint16 full_len = (PREFIX_LEN + msg_len + SUFFIX_LEN);
    uint8* full_msg = malloc(full_len * sizeof(uint8));
    memcpy(full_msg, msg_prefix, PREFIX_LEN * sizeof(uint8));
    memcpy(full_msg + PREFIX_LEN, msg, msg_len * sizeof(uint8));
    memcpy(full_msg + PREFIX_LEN + msg_len, msg_suffix, SUFFIX_LEN * sizeof(uint8));

    uint16 full_msg_sz = full_len * sizeof(*full_msg);

    decaIrqStatus_t stat;
    stat = decamutexon();
    dwt_forcetrxoff();

	/* Set expected response's delay and timeout.
    As this example only handles one incoming frame with always the same delay and
    timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(40); //dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(800); // dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT*100);

    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
    dwt_writetxdata(full_msg_sz, full_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(full_msg_sz, 0, 1); /* Zero offset in TX buffer, ranging. */

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

int dataReceiveCallback(uint8 *rx_data){    
    CDC_Transmit_FS(rx_data, MAX_FRAME_LEN);
    osDelay(1);
    return 1;
}