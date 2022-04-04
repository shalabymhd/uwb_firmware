/**
  ******************************************************************************
  * @file    ranging.c
  * @brief   This file provides code for UWB ranging, such as one-way
  *          ranging (OWR) and two-way ranging (TWR).
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ranging.h"
#include "bias.h"
#include "messaging.h"
#include <assert.h>

extern osThreadId twrInterruptTaskHandle;

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 1000

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 1000

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
static uint8 rx_buffer[MAX_FRAME_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 150 //300
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW1000's delayed TX function. This includes the
 * frame length of approximately 2.66 ms with above configuration. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS 1500 //3100 TODO: Tune this
/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 2000 //2700
/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 8

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW1000's delayed TX function. This includes the
 * frame length of approximately 2.46 ms with above configuration. */
#define POLL_RX_TO_RESP_TX_DLY_UUS 400 //2750
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define RESP_TX_TO_FINAL_RX_DLY_UUS 40 //500
/* Receive final timeout. See NOTE 5 below. */
#define FINAL_RX_TIMEOUT_UUS 600 //3300

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 tx_poll_msg[]  = {0x41, 0x88, 0xA, 0, BOARD_ID, 0, 0, 0, 0, 0};
static uint8 rx_resp_msg[]  = {0x41, 0x88, 0xB, 0, 0, BOARD_ID, 0, 0};
static uint8 tx_final_msg[] = {0x41, 0x88, 0xC, 0, BOARD_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 rx_poll_msg[]  = {0x41, 0x88, 0xA, 0, 0, BOARD_ID, 0, 0};
static uint8 tx_resp_msg[]  = {0x41, 0x88, 0xB, 0, BOARD_ID, 0, 0, 0};
static uint8 rx_final_msg[] = {0x41, 0x88, 0xC, 0, 0, BOARD_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN (6)
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_TYPE_IDX (2)
#define ALL_MSG_SEQ_IDX (3)
#define ALL_TX_BOARD_IDX (4)
#define ALL_RX_BOARD_IDX (5)
#define TX_POLL_TARG_MEAS_IDX (6) // the index associated with the target meas boolean
#define TX_POLL_MULT_TWR_IDX (7) // the index associated with the multiplicative TWR boolean
#define FINAL_SIGNAL1_TS_IDX (6)
#define FINAL_SIGNAL2_TS_IDX (10)
#define FINAL_SIGNAL3_TS_IDX (14)

/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547

/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;

/* Declaration of static functions. */
static void rx_ok_cb(const dwt_cb_data_t *cb_data);

/* Default inter-frame delay period, in milliseconds. */
#define DFLT_TX_DELAY_MS 1000
/* Inter-frame delay period in case of RX timeout, in milliseconds.
 * In case of RX timeout, assume the receiver is not present and lower the rate of blink transmission. */
#define RX_TO_TX_DELAY_MS 3000
/* Inter-frame delay period in case of RX error, in milliseconds.
 * In case of RX error, assume the receiver is present but its response has not been received for any reason and retry blink transmission immediately. */
#define RX_ERR_TX_DELAY_MS 0

/* Passive listening toggle */
static bool passive_listening = 0;

/* */
void uwbFrameHandler(void){
    uint8 msg_type = rx_buffer[ALL_MSG_TYPE_IDX];

    switch (msg_type)
    {
        case 0xA:{
            twrReceiveCallback();
        }
        case 0xB:{
            usb_print("WARNING 0xB: TWR message is firing an interrupt when it shouldnt be.\r\n");
        }
        case 0xC:{
            usb_print("WARNING 0xC: TWR message is firing an interrupt when it shouldnt be.\r\n");
        }
        case 0xD:{
            dataReceiveCallback(rx_buffer);
        }
    }
}


/* MAIN RANGING FUNCTIONS ---------------------------------------- */ 
int twrInitiateInstance(uint8_t target_id, bool target_meas_bool, uint8_t mult_twr){
    int ret;
    decaIrqStatus_t stat;
    uint64 tx1_ts;
    uint64 rx2_ts, rx3_ts;

    stat = decamutexon();
    dwt_forcetrxoff();

	/* Set expected response's delay and timeout.
     * As this example only handles one incoming frame with always the same delay and
       timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(40); //dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(800); // dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT*100);

    /* Include Target board in all communication messages. */
    tx_poll_msg[ALL_RX_BOARD_IDX] = target_id;
    rx_resp_msg[ALL_TX_BOARD_IDX] = target_id;
    tx_final_msg[ALL_RX_BOARD_IDX] = target_id;
    rx_final_msg[ALL_TX_BOARD_IDX] = target_id;

    /* Indicate whether the Target board will also compute the range measurement */
    tx_poll_msg[TX_POLL_TARG_MEAS_IDX] = target_meas_bool;    

    /* Indicate whether the multiplicative TWR will be used */
    tx_poll_msg[TX_POLL_MULT_TWR_IDX] = mult_twr;    

    /* Write frame data to DW1000 and prepare transmission. See NOTE 8 below. */
    tx_poll_msg[ALL_MSG_SEQ_IDX] = frame_seq_nb;
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); /* Zero offset in TX buffer, ranging. */

    /* Start transmission, indicating that a response is expected so that reception is 
        enabled automatically after the frame is sent and the delay set by 
        dwt_setrxaftertxdelay() has elapsed. */
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
    
    if (mult_twr){
        /* We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. See NOTE 9 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
        { };

        /* Increment frame sequence number after transmission of the poll message (modulo 256). */
        frame_seq_nb++;

        if (status_reg & SYS_STATUS_RXFCG)
        {
            uint32 frame_len;

            /* Clear good RX frame event and TX frame sent in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);

            /* A frame has been received, read it into the local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
            if (frame_len <= MAX_FRAME_LEN)
            {
                dwt_readrxdata(rx_buffer, frame_len, 0);
            }
        
            /* Check that the frame is the expected response from the companion "DS TWR responder" example.
                * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
            // rx_buffer[ALL_RX_BOARD_IDX] = target_id;
            rx_buffer[ALL_MSG_SEQ_IDX] = 0;
            if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
            {
                /* Retrieve the transmission timestamp */
                tx1_ts = get_tx_timestamp_u64();

                /* Retrieve the reception timestamp */
                rx2_ts = get_rx_timestamp_u64();

                ret = rxTimestampsDS(tx1_ts,rx2_ts,target_id,1);

                /* Retrieve the reception timestamp */
                rx3_ts = get_rx_timestamp_u64();
            }
            else{
                ret = 0;
            }
        }
        else
        {
            /* Clear RX error/timeout events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

            /* Reset RX to properly reinitialise LDE operation. */
            dwt_rxreset();

            ret = 0;
        }   
    }
    else{
        /* Await the return signal and compute the range measurement */
        ret = rxTimestampsSS(0,target_id,1);

        /* Retrieve the transmission timestamp */
        tx1_ts = get_tx_timestamp_u64();

        /* Retrieve the reception timestamp */
        rx2_ts = get_rx_timestamp_u64();
    }

    if (ret){
        // check if an additional signal is expected
        if (target_meas_bool){ 
            // send the additional signal
            if (mult_twr){
                ret = txTimestampsDS(tx1_ts, rx2_ts, rx3_ts, 1);
            }
            else{
                ret = txTimestampsSS(tx1_ts, rx2_ts, 1);
            }

            if (ret){ // TODO: allow additional signal with alternative double-sided TWR
                dwt_setpreambledetecttimeout(0);
                dwt_setrxtimeout(0);
                dwt_rxenable(DWT_START_RX_IMMEDIATE);
                decamutexoff(stat);
                return 1;
            }
        }
        else{
            dwt_setpreambledetecttimeout(0);
            dwt_setrxtimeout(0);
            dwt_rxenable(DWT_START_RX_IMMEDIATE);
            decamutexoff(stat);
            return 1;
        }
    }

    dwt_setpreambledetecttimeout(0);
    dwt_setrxtimeout(0);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    decamutexoff(stat);
    return 0;
}

int twrReceiveCallback(void){
    uint64 tx2_ts;
    uint64 rx1_ts;

    /* Set preamble timeout for expected frames. See NOTE 6 below. */
    dwt_setpreambledetecttimeout(PRE_TIMEOUT*100);

    /* Clear reception timeout to start next ranging process. */
    dwt_setrxtimeout(0); //dwt_setrxtimeout(2000*UUS_TO_DWT_TIME); /

    /* Retrieve the initiator's ID */
    uint8_t initiator_id = rx_buffer[ALL_TX_BOARD_IDX];
    
    /* Update all the messages to incorporate the initiator's ID */
    rx_poll_msg[ALL_TX_BOARD_IDX] = initiator_id;
    tx_resp_msg[ALL_RX_BOARD_IDX] = initiator_id;
    rx_final_msg[ALL_TX_BOARD_IDX] = initiator_id;
    tx_final_msg[ALL_RX_BOARD_IDX] = initiator_id;

    /* Retrieve the boolean defining whether a 4th signal is expected */
    bool target_meas_bool = rx_buffer[TX_POLL_TARG_MEAS_IDX];

    /* Retrieve the boolean defining whether the multiplicative TWR will be used */
    uint8_t mult_twr = rx_buffer[TX_POLL_MULT_TWR_IDX];

    /* Check that the frame is a poll sent by "DS TWR initiator" example. */
    rx_buffer[ALL_MSG_SEQ_IDX] = 0;
    if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN-1) == 0) // Not comparing expected target yet
    {
        int ret;

        /* Retrieve the reception timestamp */
        rx1_ts = get_rx_timestamp_u64();

        // If the intended target does not match the ID, passively listen on all signals and output the timestamps.
        bool bool_target = (rx_buffer[ALL_RX_BOARD_IDX] != rx_poll_msg[ALL_RX_BOARD_IDX]);
        bool bool_msg_type = (rx_buffer[ALL_MSG_TYPE_IDX] == rx_poll_msg[ALL_MSG_TYPE_IDX]);
        if (bool_target && bool_msg_type && passive_listening){
            
            ret = passivelyListen(rx1_ts, target_meas_bool);

            /* Compute received signal power */
            double Pr = retrieveFPP();
            char power[10] = {0};
            convert_float_to_string(power,Pr);
            usb_print(strcat(power,"\r\n"));

            // Infinite timeout for interrupt receiver running in the background 
            dwt_setrxtimeout(0);
            dwt_setpreambledetecttimeout(0);
            return ret;
        }
        else if(bool_target){
            dwt_setrxtimeout(0);
            dwt_setpreambledetecttimeout(0);
            return 0;
        }

        if (mult_twr){
            /* Write and send the response message. See NOTE 10 below.*/
            tx_resp_msg[ALL_MSG_SEQ_IDX] = frame_seq_nb;
            dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
            dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
            ret = dwt_starttx(DWT_START_TX_IMMEDIATE);

            /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 11 below. */
            if (ret == DWT_ERROR)
            {
                dwt_setrxtimeout(0);
                dwt_setpreambledetecttimeout(0);
                return 0;
            }

            /* Poll DW1000 until TX frame sent event set. See NOTE 9 below. */
            while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
            { };

            /* Retrieve the transmission timestamp */
            tx2_ts = get_tx_timestamp_u64();

            /* Clear TXFRS event. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

            /* Increment frame sequence number after transmission of the final message (modulo 256). */
            frame_seq_nb++;

            /* Transmit the delayed signal with the time-stamps */
            ret = txTimestampsDS(rx1_ts, tx2_ts, 0, 0);

            /* Compute received signal power */
            double Pr = retrieveFPP();
            char power[10] = {0};
            convert_float_to_string(power,Pr);
            usb_print(strcat(power,"\r\n"));
        }
        else{
            /* Transmit the delayed signal with the time-stamps */
            ret = txTimestampsSS(rx1_ts, 0, 0);

            /* Compute received signal power */
            double Pr = retrieveFPP();
            char power[10] = {0};
            convert_float_to_string(power,Pr);
            usb_print(strcat(power,"\r\n"));
        }

        if (ret){
            /* Check if an additional signal is expected to communicate the time-stamps to the target */
            if (target_meas_bool){
                /* Set expected delay and timeout for final message reception.*/
                dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
                dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

                /* Receive the additional signal and calculate the range measurement */
                if (mult_twr){
                    ret = rxTimestampsDS(rx1_ts,tx2_ts,initiator_id,0);
                }
                else{
                    ret = rxTimestampsSS(rx1_ts,initiator_id,0);
                }
                if (ret){ // TODO: allow 4th signal with double-sided TWR   
                    dwt_setpreambledetecttimeout(0);
                    dwt_setrxtimeout(0);
                    return 1;
                }
            }
            else{
                dwt_setpreambledetecttimeout(0);
                dwt_setrxtimeout(0);
                return 1;
            }
        }
    }
    
    dwt_setrxtimeout(0);
    dwt_setpreambledetecttimeout(0);
    return 0;
}

int txTimestampsSS(uint64 ts1, uint64 ts2, bool is_immediate){
    /* Set-up delayed transmission to encode the transmission time-stamp in the final message */
    int ret;
    uint8_t tx_type;

    if (is_immediate){
         /* Write all timestamps in the final message. See NOTE 11 below. */
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // tx1
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL2_TS_IDX], ts2); // rx2
        
        tx_type = DWT_START_TX_IMMEDIATE;
    }
    else{
        uint32 final_tx_time;
        uint64 final_tx_ts;

         /* Write all timestamps in the final message. See NOTE 11 below. */
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // rx1

        /* Compute final message transmission time. See NOTE 10 below. */
        final_tx_time = (ts1 + (500 * UUS_TO_DWT_TIME)) >> 8;

        dwt_setdelayedtrxtime(final_tx_time);

        /* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
        final_tx_ts = (((uint64)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        /* Write timestamp in the final message. See NOTE 11 below. */
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL2_TS_IDX], final_tx_ts);

        tx_type = DWT_START_TX_DELAYED;
    }

    /* Write and send final message. See NOTE 8 below. */
    tx_final_msg[ALL_MSG_SEQ_IDX] = frame_seq_nb;
    dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_final_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
    ret = dwt_starttx(tx_type);

    /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 12 below. */
    if (ret == DWT_SUCCESS)
    {
        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

        /* Increment frame sequence number after transmission of the final message (modulo 256). */
        frame_seq_nb++;

        return 1;
    }
    
    return 0;
}

int txTimestampsDS(uint64 ts1, uint64 ts2, uint64 ts3, bool is_immediate){
    /* Set-up delayed transmission to encode the transmission time-stamp in the final message */
    int ret;
    uint8_t tx_type;

    if (is_immediate){
         /* Write all timestamps in the final message. See NOTE 11 below. */
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // tx1 
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL2_TS_IDX], ts2); // rx2 
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL3_TS_IDX], ts3); // rx3
        
        tx_type = DWT_START_TX_IMMEDIATE;
    }
    else{
        uint32 final_tx_time;
        uint64 final_tx_ts;

         /* Write all timestamps in the final message. See NOTE 11 below. */
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // rx1
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL2_TS_IDX], ts2); // tx2

        /* Compute final message transmission time. See NOTE 10 below. */
        final_tx_time = (ts1 + (500 * UUS_TO_DWT_TIME)) >> 8;

        dwt_setdelayedtrxtime(final_tx_time);

        /* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
        final_tx_ts = (((uint64)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        /* Write timestamp in the final message. See NOTE 11 below. */
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL3_TS_IDX], final_tx_ts);

        tx_type = DWT_START_TX_DELAYED;
    }

    /* Write and send final message. See NOTE 8 below. */
    tx_final_msg[ALL_MSG_SEQ_IDX] = frame_seq_nb;
    dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_final_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
    ret = dwt_starttx(tx_type);

    /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 12 below. */
    if (ret == DWT_SUCCESS)
    {
        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

        /* Increment frame sequence number after transmission of the final message (modulo 256). */
        frame_seq_nb++;

        return 1;
    }
    
    return 0;
}

int rxTimestampsSS(uint64 ts1, uint8_t neighbour_id, bool is_initiator){
    /* String used to display measured distance on UART. */
    uint32 frame_len;
    double Ra, Db;
    uint32 rx1_ts, tx2_ts;
    uint32 tx1_ts, rx2_ts;
    int64 tof_dtu;

    dwt_setrxtimeout(0);
    // dwt_setpreambledetecttimeout(0);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    /* Poll for reception of expected "final" frame or error/timeout. See NOTE 8 below. */
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    { };

    /* Increment frame sequence number after transmission of the response message (modulo 256). */
    frame_seq_nb++;

    if (status_reg & SYS_STATUS_RXFCG)
    {
        /* Clear good RX frame event and TX frame sent in the DW1000 status register. */
        // dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);

        /* A frame has been received, read it into the local buffer. */
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
        // if (frame_len <= MAX_FRAME_LEN)
        if (frame_len <= 1024)
        {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }

        /* Check that the frame is a final message sent by "DS TWR initiator" example.
            * As the sequence number field of the frame is not used in this example, it can be zeroed to ease the validation of the frame. */
        rx_buffer[ALL_MSG_SEQ_IDX] = 0;
        if (memcmp(rx_buffer, rx_final_msg, ALL_MSG_COMMON_LEN) == 0)
        {
            if (is_initiator){
                /* Get timestamps embedded in the final message. */
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &rx1_ts);
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &tx2_ts);
                
                tx1_ts = (uint32)get_tx_timestamp_u64();
                rx2_ts = (uint32)get_rx_timestamp_u64();
            }
            else{
                /* Get timestamps embedded in the final message. */
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &tx1_ts);
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &rx2_ts);
            
                rx1_ts = (uint32)ts1;
                tx2_ts = (uint32)get_tx_timestamp_u64();                
            }
            

            /* Compute time of flight. 32-bit subtractions give correct answers even if clock has wrapped. See NOTE 12 below. */            
            Ra = (double)(rx2_ts - tx1_ts);
            Db = (double)(tx2_ts - rx1_ts);
            tof_dtu = (int64)((Ra - Db) / (2)); // Standard single-sided TWR
            
            tof = tof_dtu * DWT_TIME_UNITS;
            distance = tof * SPEED_OF_LIGHT;

            /* Compute received signal power */
            double Pr = retrieveFPP();
            char power[10] = {0};
            convert_float_to_string(power,Pr);
            usb_print(strcat(power,"\r\n"));

            /* Display computed distance. */
            char dist_str[10] = {0};
            convert_float_to_string(dist_str,distance);
            char response[100];
            
            sprintf(response, "R05|%d|%s|%lu|%lu|%lu|%lu\r\n",
                    neighbour_id, dist_str,
                    tx1_ts, rx1_ts,
                    tx2_ts, rx2_ts);
            usb_print(response); // TODO: will this response ever be sent without a USB command?
            
            dwt_setrxtimeout(0);
            dwt_setpreambledetecttimeout(0);
            return 1;
        }
    }
    else
    {
        /* Clear RX error/timeout events in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

        /* Reset RX to properly reinitialise LDE operation. */
        dwt_rxreset();
    }

    return 0;
}

int rxTimestampsDS(uint64 ts1, uint64 ts2, uint8_t neighbour_id, bool is_initiator){
    /* String used to display measured distance on UART. */
    uint32 frame_len;
    double Ra1, Ra2, Db1, Db2;
    uint32 rx1_ts, tx2_ts, tx3_ts;
    uint32 tx1_ts, rx2_ts, rx3_ts;
    int64 tof_dtu;

    if (is_initiator){
        /* Compute received signal power */
        double Pr = retrieveFPP();
        char power[10] = {0};
        convert_float_to_string(power,Pr);
        usb_print(strcat(power,"\r\n"));
    }

    // dwt_setrxtimeout(0);
    // dwt_setpreambledetecttimeout(0);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    /* Poll for reception of expected "final" frame or error/timeout. See NOTE 8 below. */
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    { };

    /* Increment frame sequence number after transmission of the response message (modulo 256). */
    frame_seq_nb++;

    if (status_reg & SYS_STATUS_RXFCG)
    {
        /* Clear good RX frame event and TX frame sent in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);

        /* A frame has been received, read it into the local buffer. */
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
        // if (frame_len <= MAX_FRAME_LEN)
        if (frame_len <= 1024)
        {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }

        /* Check that the frame is a final message sent by "DS TWR initiator" example.
            * As the sequence number field of the frame is not used in this example, it can be zeroed to ease the validation of the frame. */
        rx_buffer[ALL_MSG_SEQ_IDX] = 0;
        if (memcmp(rx_buffer, rx_final_msg, ALL_MSG_COMMON_LEN) == 0)
        {
            if (is_initiator){
                /* Get timestamps embedded in the final message. */
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &rx1_ts);
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &tx2_ts);
                
                /* Get the transmission time-stamp of the final signal from the neighbour */
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL3_TS_IDX], &tx3_ts);

                tx1_ts = (uint32)ts1;
                rx2_ts = (uint32)ts2;

                /* Get the reception time-stamp of the final signal */
                rx3_ts = (uint32)get_rx_timestamp_u64();    
            }
            else{
                /* Get timestamps embedded in the final message. */
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &tx1_ts);
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &rx2_ts);
                
                /* Get the transmission time-stamp of the final signal from the neighbour */
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL3_TS_IDX], &rx3_ts);

                rx1_ts = (uint32)ts1;
                tx2_ts = (uint32)ts2;

                /* Get the transmission time-stamp of the final signal */
                tx3_ts = (uint32)get_tx_timestamp_u64();      
            }            

            /* Compute time of flight. 32-bit subtractions give correct answers even if clock has wrapped. See NOTE 12 below. */            
            Ra1 = (double)(rx2_ts - tx1_ts);
            Ra2 = (double)(rx3_ts - rx2_ts);
            Db1 = (double)(tx2_ts - rx1_ts);
            Db2 = (double)(tx3_ts - tx2_ts);
            tof_dtu = (int64)((Ra1*Db2 - Ra2*Db1) / (Ra2 + Db2)); // Reversed alternative double-sided TWR
            // tof_dtu = (int64)(0.5*(Ra1 - Ra2/Db2*Db1)); // Reversed alternative double-sided TWR
           
            tof = tof_dtu * DWT_TIME_UNITS;
            distance = tof * SPEED_OF_LIGHT;

            /* Display computed distance. */
            char dist_str[10] = {0};
            convert_float_to_string(dist_str,distance);
            char response[100];
            sprintf(response, "R05|%d|%s|%lu|%lu|%lu|%lu|%lu|%lu\r\n",
                    neighbour_id, dist_str,
                    tx1_ts, rx1_ts,
                    tx2_ts, rx2_ts,
                    tx3_ts, rx3_ts);
            usb_print(response); // TODO: will this response ever be sent without a USB command?
            
            dwt_setrxtimeout(0);
            dwt_setpreambledetecttimeout(0);
            return 1;
        }
    }
    else
    {
        /* Clear RX error/timeout events in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

        /* Reset RX to properly reinitialise LDE operation. */
        dwt_rxreset();
    }

    return 0;
}

int passivelyListen(uint32_t rx_ts1, bool four_signals){
    /* Have received first signal in a TWR transaction. 2 or 3 more signals expected. */
    bool success;
    uint8_t master_id, slave_id;
    uint32_t rx_ts2, tx_ts1, tx_ts2;

    // Retrieve IDs of tags involved in the TWR transaction
    master_id = rx_buffer[ALL_TX_BOARD_IDX];
    slave_id = rx_buffer[ALL_RX_BOARD_IDX];

    /* --------------------- SIGNAL 2: Slave to Master --------------------- */
    success = timestampReceivedFrame(&rx_ts2, ALL_RX_BOARD_IDX, master_id,
                                     ALL_TX_BOARD_IDX, slave_id, 0);
    if (success == 0){
        return 0;
    }
    
    /* --------------------- SIGNAL 3: Slave to Master --------------------- 
    The transmission time-stamp of Signal 2 is embedded in the received frame */
    success = timestampReceivedFrame(&tx_ts2, ALL_RX_BOARD_IDX, master_id,
                                     ALL_TX_BOARD_IDX, slave_id, 1);
    if (success == 0){
        return 0;
    }

    /* --------------------- SIGNAL 4: Master to Slave --------------------- 
    The transmission time-stamp of Signal 1 is embedded in the received frame */
    // Check if fourth signal is expected.
    if (four_signals){
        success = timestampReceivedFrame(&tx_ts1, ALL_TX_BOARD_IDX, master_id,
                                         ALL_RX_BOARD_IDX, slave_id, 1);
        if (success == 0){
            return 0;
        }
    }

    /* --------------------- Output Time-stamps --------------------- */
    char output[60];
    sprintf(output,"R99|%lu|%lu|%lu|%lu\r\n",tx_ts1,rx_ts1,tx_ts2,rx_ts2);
    usb_print(output);
    return 1;
}

bool timestampReceivedFrame(uint32_t *ts, uint8_t master_idx, uint8_t master_id,
                            uint8_t slave_idx, uint8_t slave_id, bool final_signal){
    uint32 frame_len;
    bool bool_base, bool_master, bool_slave;
    
    // Re-enable RX
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    /* Poll for reception of expected frame or error/timeout. */
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    { };

    if (status_reg & SYS_STATUS_RXFCG)
    {
        /* Clear good RX frame event and TX frame sent in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);

        /* A frame has been received, read it into the local buffer. */
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
        if (frame_len <= MAX_FRAME_LEN)
        {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }
    
        /* Check that the frame is the expected response.
            * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
        rx_buffer[ALL_MSG_SEQ_IDX] = 0;
        bool_base = (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN-2) == 0);
        bool_master = rx_buffer[master_idx] == master_id;
        bool_slave = rx_buffer[slave_idx] == slave_id;
        if (bool_base && bool_master && bool_slave){
            /* Retrieve the reception timestamp */
            if (final_signal){
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], ts);
            }
            else{
                *ts = get_rx_timestamp_u64();
            }
            return 1;
        }
        else{
            return 0;
        }
    }
    else{
        return 0;
    }
}

void setPassiveToggle(bool toggle){
    passive_listening = toggle;
}

void uwbReceiveInterruptInit(){
    /* Install DW1000 IRQ handler. */
    port_set_deca_isr(dwt_isr);

    /* Register RX call-back. */
    dwt_setcallbacks(NULL, &rx_ok_cb, NULL, NULL);

    /* Enable wanted interrupts (TX confirmation, RX good frames, RX timeouts and RX errors). */
    // dwt_setinterrupt(DWT_INT_RFCG | DWT_INT_RFTO | DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFSL | DWT_INT_SFDT, 1);
    dwt_setinterrupt(DWT_INT_RFCG, 1);

    /* Set delay to turn reception on after transmission of the frame. See NOTE 2 below. */
    dwt_setrxaftertxdelay(60);

    /* Set response frame timeout. */
    dwt_setrxtimeout(0);

    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_ok_cb()
 *
 * @brief Callback to process RX good frame events
 *
 * @param  cb_data  callback data
 *
 * @return  none
 */
static void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
    int i;

    /* Clear local RX buffer to avoid having leftovers from previous receptions. 
    This is not necessary but is included here to aid reading the RX
    buffer. */
    for (i = 0 ; i < MAX_FRAME_LEN; i++ )
    {
        rx_buffer[i] = 0;
    }

    /* A frame has been received, copy it to our local buffer. */
    if (cb_data->datalength <= MAX_FRAME_LEN)
    {
        dwt_readrxdata(rx_buffer, cb_data->datalength, 0);
    }

    osThreadResume(twrInterruptTaskHandle);
}