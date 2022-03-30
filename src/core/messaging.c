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

/* Prefix and Suffix of Message SENT over UWB */
#define PREFIX_LEN 4
#define SUFFIX_LEN 2
#define MAX_MSG_LEN (MAX_FRAME_LEN - PREFIX_LEN - SUFFIX_LEN)
uint8 msg_prefix[] = {0x41, 0x88, 0xD, 0};
uint8 msg_suffix[] = {0, 0};

/* Prefix and Suffix of Message SENT over USB */
char resp_prefix[] = "R06|";
char resp_suffix[] = "\r\n";

/*! -------------------------------------------------------------------------
 * @fn broadcast(uint8* msg, size_t msg_len)
 *
 * @brief Send an arbirary array of bytes over UWB, and do not expect response.
 *
 * Parameters
 * ----------
 * @param msg - pointer to an array of bytes to send over UWB
 * @param msg_len - number of bytes in msg to send
 */

int broadcast(uint8* msg, size_t msg_len){

    if (msg_len > MAX_MSG_LEN){
        usb_print("ERROR: Requested to send message over UWB that is too long.");
        return 0;
    }
    else{
        // Concatenate arrays into one large array (memory duplication here)
        uint16 full_len = (PREFIX_LEN + msg_len + SUFFIX_LEN);
        uint8* full_msg = malloc(full_len * sizeof(uint8));
        memcpy(full_msg, msg_prefix, PREFIX_LEN * sizeof(uint8));
        memcpy(full_msg + PREFIX_LEN, msg, msg_len * sizeof(uint8));
        memcpy(full_msg + PREFIX_LEN + msg_len, msg_suffix, SUFFIX_LEN * sizeof(uint8));

        uint16 full_msg_sz = full_len * sizeof(*full_msg);

        //CDC_Transmit_FS(full_msg, full_len);
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
        dwt_writetxfctrl(full_msg_sz, 0, 0); /* Zero offset in TX buffer, ranging. */

        /* Start transmission, indicating that a response is expected so that reception is 
            enabled automatically after the frame is sent and the delay set by 
            dwt_setrxaftertxdelay() has elapsed. */
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);


        dwt_setpreambledetecttimeout(0);
        dwt_setrxtimeout(0);
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        decamutexoff(stat);
        free(full_msg);
        return 1;
    }
}


/*! -------------------------------------------------------------------------
 * @fn dataReceiveCallback(uint8 *rx_data)
 *
 * @brief This function gets called whenever a message type of 0xD is 
 * detected.
 *
 * Parameters
 * ----------
 * @param msg - pointer to an array of bytes to send over UWB
 * @param msg_len - number of bytes in msg to send
 */
int dataReceiveCallback(uint8 *rx_data){    

    // Pointer to RX_DATA with prefix removed.
    char* rx_data_no_prefix = (char*) rx_data + PREFIX_LEN;

    // Read data until the first '\0' appears (end of string)
    // Since the suffix is just {0, 0}, this will remove the suffix too.
    uint16 data_len = strlen((char*) rx_data_no_prefix);

    // Get the length of the full string to be transmitted over USB.
    uint16 full_len = (strlen(resp_prefix) + data_len + strlen(resp_suffix));

    // Concatenate arrays into one large array
    char full_msg[full_len];
    sprintf(full_msg, "%s%s%s", resp_prefix, rx_data_no_prefix, resp_suffix);

    // Transmit the final concatenated array over USB
    usb_print(full_msg);

    osDelay(1);
    return 1;
}