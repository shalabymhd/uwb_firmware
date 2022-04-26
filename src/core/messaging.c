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
#define PREFIX_LEN 4 // in number of bytes
#define SUFFIX_LEN 2 // in number of bytes
#define MAX_MSG_LEN (MAX_FRAME_LEN - PREFIX_LEN - SUFFIX_LEN)
uint8 msg_prefix[] = {0x41, 0x88, 0xD, 0};
uint8 msg_suffix[] = {0, 0};

/* Prefix and Suffix of Message SENT over USB */
char resp_prefix[] = "S06|";
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

int broadcast(uint8* msg, uint16_t msg_len){

    if (msg_len > (MAX_FRAME_LEN - PREFIX_LEN - SUFFIX_LEN - 2)){
        usb_print("ERROR: Requested to send message over UWB that is too long.");
        return 0;
    }
    else{
        // Concatenate arrays into one large array (memory duplication here)
        uint16_t full_len = (PREFIX_LEN + 2 + msg_len + SUFFIX_LEN);
        uint8_t full_msg[MAX_FRAME_LEN];
        memset(full_msg, 0, MAX_FRAME_LEN);
        memcpy(full_msg                           , msg_prefix, PREFIX_LEN);
        memcpy(full_msg + PREFIX_LEN              , &msg_len  , 2);
        memcpy(full_msg + PREFIX_LEN + 2          , msg       , msg_len);
        memcpy(full_msg + PREFIX_LEN + 2 + msg_len, msg_suffix, SUFFIX_LEN);

        //CDC_Transmit_FS(full_msg, full_len);
        decaIrqStatus_t stat;
        stat = decamutexon();
        dwt_forcetrxoff();

        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
        dwt_writetxdata(full_len, full_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(full_len, 0, 0); /* Zero offset in TX buffer, ranging. */

        /* Start transmission. */
        dwt_starttx(DWT_START_TX_IMMEDIATE);

        dwt_setpreambledetecttimeout(0);
        dwt_setrxtimeout(0);
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        decamutexoff(stat);
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
int dataReceiveCallback(uint8_t *rx_data){    

    // Pointer to RX_DATA with prefix removed.
    uint8_t * rx_data_no_prefix = rx_data + PREFIX_LEN;
    // First two bytes of byte array are always the length (in number of bytes)
    // of the upcoming byte array
    uint16_t data_len;
    memcpy(&data_len, rx_data_no_prefix, 2);

    // Get the length of the full string to be transmitted over USB.
    uint16_t prefix_len = 4;
    uint16_t suffix_len = 2;
    uint16_t full_len = (prefix_len + 2 + data_len + suffix_len);

    // Concatenate arrays into one large array
    uint8_t full_msg[full_len];
    memcpy(&full_msg[0], resp_prefix, prefix_len);
    memcpy(&full_msg[0] + prefix_len, rx_data_no_prefix, 2 + data_len);
    memcpy(&full_msg[0] + prefix_len + 2 + data_len, resp_suffix, suffix_len);
    

    // Transmit the final concatenated array over USB
    CDC_Transmit_FS(full_msg, full_len);
    return 1;
}