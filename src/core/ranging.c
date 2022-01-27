/**
  ******************************************************************************
  * @file    ranging.c
  * @brief   This file provides code for UWB ranging, such as one-way
  *          ranging (OWR) and two-way ranging (TWR).
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ranging.h"

extern osThreadId twrInterruptTaskHandle;

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 1000

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 1000

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 40
static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 150 //300
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW1000's delayed TX function. This includes the
 * frame length of approximately 2.66 ms with above configuration. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS 500 //3100
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
static uint8 tx_poll_msg[] = {0x41, 0x88, 0, BOARD_ID, 0, 0xA, 0, 0, 0};
static uint8 rx_resp_msg[] = {0x41, 0x88, 0, 0, BOARD_ID, 0xB, 0, 0};
static uint8 tx_final_msg[] = {0x41, 0x88, 0, BOARD_ID, 0, 0xC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 rx_poll_msg[] = {0x41, 0x88, 0, 0, BOARD_ID, 0xA, 0, 0};
static uint8 tx_resp_msg[] = {0x41, 0x88, 0, BOARD_ID, 0, 0xB, 0, 0};
static uint8 rx_final_msg[] = {0x41, 0x88, 0, 0, BOARD_ID, 0xC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN (5)
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SEQ_IDX 2
#define ALL_TX_BOARD_IDX (3)
#define ALL_RX_BOARD_IDX (4)
#define ALL_MSG_TYPE_IDX (5)
#define TX_POLL_TARG_MEAS_IDX (6) // the index associated with the target meas boolean
#define FINAL_POLL_TX_TS_IDX (6)
#define FINAL_RESP_RX_TS_IDX (10)
#define FINAL_FINAL_TX_TS_IDX (14)

/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547

/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;

/* Declaration of static functions. */
static void rx_ok_cb(const dwt_cb_data_t *cb_data);
static void rx_to_cb(const dwt_cb_data_t *cb_data);
static void rx_err_cb(const dwt_cb_data_t *cb_data);

/* Default inter-frame delay period, in milliseconds. */
#define DFLT_TX_DELAY_MS 1000
/* Inter-frame delay period in case of RX timeout, in milliseconds.
 * In case of RX timeout, assume the receiver is not present and lower the rate of blink transmission. */
#define RX_TO_TX_DELAY_MS 3000
/* Inter-frame delay period in case of RX error, in milliseconds.
 * In case of RX error, assume the receiver is present but its response has not been received for any reason and retry blink transmission immediately. */
#define RX_ERR_TX_DELAY_MS 0

/* MAIN RANGING FUNCTIONS ---------------------------------------- */ 
int twrInitiateInstance(uint8_t target_ID, bool target_meas_bool){
    decaIrqStatus_t stat;
    uint64 tx_ts;
    uint64 rx_ts;

    stat = decamutexon();
    dwt_forcetrxoff();

	/* Set expected response's delay and timeout.
     * As this example only handles one incoming frame with always the same delay and
       timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(40); //dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(800); // dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT*100);

    /* Include Target board in all communication messages. */
    tx_poll_msg[ALL_RX_BOARD_IDX] = target_ID;
    rx_resp_msg[ALL_TX_BOARD_IDX] = target_ID;
    tx_final_msg[ALL_RX_BOARD_IDX] = target_ID;
    rx_final_msg[ALL_TX_BOARD_IDX] = target_ID;

    /* Indicate whether the Target board will also compute the range measurement */
    tx_poll_msg[TX_POLL_TARG_MEAS_IDX] = target_meas_bool;    

    /* Write frame data to DW1000 and prepare transmission. See NOTE 8 below. */
    tx_poll_msg[ALL_MSG_SEQ_IDX] = frame_seq_nb;
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); /* Zero offset in TX buffer, ranging. */

    /* Start transmission, indicating that a response is expected so that reception is 
        enabled automatically after the frame is sent and the delay set by 
        dwt_setrxaftertxdelay() has elapsed. */
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
    
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
        if (frame_len <= RX_BUF_LEN)
        {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }
    
        /* Check that the frame is the expected response from the companion "DS TWR responder" example.
            * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
        // rx_buffer[ALL_RX_BOARD_IDX] = target_ID;
        rx_buffer[ALL_MSG_SEQ_IDX] = 0;
        if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
        {
            /* Retrieve the transmission timestamp */
            tx_ts = get_tx_timestamp_u64();

            /* Retrieve the reception timestamp */
            rx_ts = get_rx_timestamp_u64();

            /* Await the third signal and compute the range measurement */
            if (rxFinal(tx_ts,rx_ts)){
                if (target_meas_bool){
                    if (txFinal(tx_ts,rx_ts)){   
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
        }
    }
    else
    {
        /* Clear RX error/timeout events in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

        /* Reset RX to properly reinitialise LDE operation. */
        dwt_rxreset();
    }

    dwt_setpreambledetecttimeout(0);
    dwt_setrxtimeout(0);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    decamutexoff(stat);
    return 0;
}

int txFinal(uint64 poll_tx_ts, uint64 resp_rx_ts){
    uint32 final_tx_time;
    int ret;

    /* Compute final message transmission time. See NOTE 10 below. */
    // final_tx_time = (resp_rx_ts + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
    if (resp_rx_ts > poll_tx_ts){
        final_tx_time = (resp_rx_ts + (600 * UUS_TO_DWT_TIME)) >> 8;
    }
    else{
        final_tx_time = (poll_tx_ts + (600 * UUS_TO_DWT_TIME)) >> 8;
    }
    dwt_setdelayedtrxtime(final_tx_time);

    // /* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
    // final_tx_ts = (((uint64)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

    /* Write all timestamps in the final message. See NOTE 11 below. */
    final_msg_set_ts(&tx_final_msg[FINAL_POLL_TX_TS_IDX], poll_tx_ts);
    final_msg_set_ts(&tx_final_msg[FINAL_RESP_RX_TS_IDX], resp_rx_ts);
    // final_msg_set_ts(&tx_final_msg[FINAL_FINAL_TX_TS_IDX], final_tx_ts);

    /* Write and send final message. See NOTE 8 below. */
    tx_final_msg[ALL_MSG_SEQ_IDX] = frame_seq_nb;
    dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_final_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
    // ret = dwt_starttx(DWT_START_TX_DELAYED);
    ret = dwt_starttx(DWT_START_TX_IMMEDIATE);

    /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 12 below. */
    if (ret == DWT_SUCCESS)
    {
        // usb_print("Final message transmitted. \n");

        /* Poll DW1000 until TX frame sent event set. See NOTE 9 below. */
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
        { };

        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

        /* Increment frame sequence number after transmission of the final message (modulo 256). */
        frame_seq_nb++;

        return 1;
    }
    
    return 0;
}

int twrReceiveCallback(void){
    uint64 tx_ts;
    uint64 rx_ts;

    /* Set preamble timeout for expected frames. See NOTE 6 below. */
    dwt_setpreambledetecttimeout(PRE_TIMEOUT*100);

    /* Clear reception timeout to start next ranging process. */
    dwt_setrxtimeout(2000*UUS_TO_DWT_TIME); //dwt_setrxtimeout(0);

    /* Extract the initiator's ID */
    uint8_t initiator_ID = rx_buffer[ALL_TX_BOARD_IDX];
    
    /* Update all the messages to incorporate the initiator's ID */
    rx_poll_msg[ALL_TX_BOARD_IDX] = initiator_ID;
    tx_resp_msg[ALL_RX_BOARD_IDX] = initiator_ID;
    rx_final_msg[ALL_TX_BOARD_IDX] = initiator_ID;
    tx_final_msg[ALL_RX_BOARD_IDX] = initiator_ID;

    /* Extract the boolean defining whether a 4th signal is expected */
    bool target_meas_bool = rx_buffer[TX_POLL_TARG_MEAS_IDX];

    /* Check that the frame is a poll sent by "DS TWR initiator" example. */
    rx_buffer[ALL_MSG_SEQ_IDX] = 0;
    if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN) == 0)
    {
        uint32 resp_tx_time;
        int ret;

        /* Retrieve the reception timestamp */
        rx_ts = get_rx_timestamp_u64();
    
        /* Set send time for response. See NOTE 9 below. */
        // resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        resp_tx_time = (rx_ts + (450 * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(resp_tx_time);
        // dwt_forcetrxoff();

        /* Write and send the response message. See NOTE 10 below.*/
        tx_resp_msg[ALL_MSG_SEQ_IDX] = frame_seq_nb;
        dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
        ret = dwt_starttx(DWT_START_TX_DELAYED);

        /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 11 below. */
        if (ret == DWT_ERROR)
        {
            dwt_setrxtimeout(0);
            dwt_setpreambledetecttimeout(0);
            return 0;
        }

        // osDelay(1);
        /* Poll DW1000 until TX frame sent event set. See NOTE 9 below. */
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
        { };

        /* Retrieve the transmission timestamp */
        tx_ts = get_tx_timestamp_u64();

        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

        /* Increment frame sequence number after transmission of the final message (modulo 256). */
        frame_seq_nb++;

        if (txFinal(tx_ts, rx_ts)){
            if (target_meas_bool){

                // /* Set expected delay and timeout for final message reception. See NOTE 4 and 5 below. */
                dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
                dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

                if (rxFinal(tx_ts, rx_ts)){   
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

int rxFinal(uint64 poll_rx_ts, uint64 resp_tx_ts){
    /* String used to display measured distance on UART. */
    char dist_str[10] = {0};
    uint32 frame_len;

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
        // if (frame_len <= RX_BUF_LEN)
        if (frame_len <= 1024)
        {
            dwt_readrxdata(rx_buffer, frame_len, 0);
        }

        /* Check that the frame is a final message sent by "DS TWR initiator" example.
            * As the sequence number field of the frame is not used in this example, it can be zeroed to ease the validation of the frame. */
        rx_buffer[ALL_MSG_SEQ_IDX] = 0;
        if (memcmp(rx_buffer, rx_final_msg, ALL_MSG_COMMON_LEN) == 0)
        {
            uint32 poll_tx_ts, resp_rx_ts, final_tx_ts;
            uint32 poll_rx_ts_32, resp_tx_ts_32;
            // uint32 final_rx_ts_32;
            double Ra, Db;
            // double Rb, Da;
            int64 tof_dtu;

            /* Get timestamps embedded in the final message. */
            final_msg_get_ts(&rx_buffer[FINAL_POLL_TX_TS_IDX], &poll_tx_ts);
            final_msg_get_ts(&rx_buffer[FINAL_RESP_RX_TS_IDX], &resp_rx_ts);
            // final_msg_get_ts(&rx_buffer[FINAL_FINAL_TX_TS_IDX], &final_tx_ts);

            /* Compute time of flight. 32-bit subtractions give correct answers even if clock has wrapped. See NOTE 12 below. */
            poll_rx_ts_32 = (uint32)poll_rx_ts;
            resp_tx_ts_32 = (uint32)resp_tx_ts;
            // final_rx_ts_32 = (uint32)final_rx_ts;
            Ra = (double)(poll_tx_ts - resp_rx_ts);
            // Rb = (double)(final_rx_ts_32 - resp_tx_ts_32);
            // Da = (double)(final_tx_ts - resp_rx_ts);
            Db = (double)(resp_tx_ts_32 - poll_rx_ts_32);
            // tof_dtu = (int64)((Ra * Rb - Da * Db) / (Ra + Rb + Da + Db));
            tof_dtu = (int64)((Db - Ra) / (2));

            tof = tof_dtu * DWT_TIME_UNITS;
            distance = tof * SPEED_OF_LIGHT;

            /* Display computed distance. */
            convert_float_to_string(dist_str,distance);
            usb_print(dist_str);
            
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

    /* Clear local RX buffer to avoid having leftovers from previous receptions. This is not necessary but is included here to aid reading the RX
     * buffer. */
    for (i = 0 ; i < RX_BUF_LEN; i++ )
    {
        rx_buffer[i] = 0;
    }

    /* A frame has been received, copy it to our local buffer. */
    if (cb_data->datalength <= RX_BUF_LEN)
    {
        dwt_readrxdata(rx_buffer, cb_data->datalength, 0);
    }

    osThreadResume(twrInterruptTaskHandle);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_to_cb()
 *
 * @brief Callback to process RX timeout events
 *
 * @param  cb_data  callback data
 *
 * @return  none
 */
static void rx_to_cb(const dwt_cb_data_t *cb_data)
{
    usb_print("RX Timeout!\r\n");

    dwt_rxenable(DWT_START_RX_IMMEDIATE); 
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_err_cb()
 *
 * @brief Callback to process RX error events
 *
 * @param  cb_data  callback data
 *
 * @return  none
 */
static void rx_err_cb(const dwt_cb_data_t *cb_data)
{
    usb_print("RX Error!\r\n");

    dwt_rxenable(DWT_START_RX_IMMEDIATE); 
}