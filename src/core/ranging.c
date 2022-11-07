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
#include "cmsis_os.h"

extern osThreadId twrInterruptTaskHandle;

typedef struct {
    uint8_t msg[MAX_FRAME_LEN];
    uint32_t len;
} UwbMsg;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
static uint8 rx_buffer[MAX_FRAME_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 8

/* This is the delay from the end of the frame transmission to the enable of the
 * receiver, as programmed for the DW1000's wait for response feature. */
#define RESP_TX_TO_FINAL_RX_DLY_UUS 40 //500
/* Receive final timeout. See NOTE 5 below. */
#define FINAL_RX_TIMEOUT_UUS 600 //3300

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 tx_poll_msg[10]  = {0x41, 0x88, 0xA};
static uint8 rx_resp_msg[8]  = {0x41, 0x88, 0xB};
static uint8 tx_final_msg[34] = {0x41, 0x88, 0xC};

/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 rx_poll_msg[8]  = {0x41, 0x88, 0xA};
static uint8 tx_resp_msg[8]  = {0x41, 0x88, 0xB};
static uint8 rx_final_msg[34] = {0x41, 0x88, 0xC};

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
#define FINAL_FPP_IDX (18) // First path power, as per Section 4.7.1 in the User Manual. Float. 
#define FINAL_RXP_IDX (22) // Received power, as per Section 4.7.2 in the User Manual. Float.
#define FINAL_STD_IDX (26) // Standard deviation of RX noise, from Register 0x12. 2 bits only
#define FINAL_SKEW_IDX (28) // Clock skew measurement from the carrier integrator, 
                            // Section 5.86 in the DW software API guide.

/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547

/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;

/* Declaration of static functions. */
static void rx_ok_cb(const dwt_cb_data_t *cb_data);

/* Passive listening toggle */
static bool passive_listening = 0;

/* Second-response delay in DS-TWR */
static uint16 tx3_delay = 1500;

static osMailQDef(UwbMsgBox, USB_QUEUE_SIZE, UwbMsg); 
static osMailQId UwbMsgBox;  

/**
 * @brief Initialization routine for UWB interrupt handling. This function is
 * called once on startup.
 * 
 */
void ranging_init(){

    /* Load board IDs into ranging frames */
    tx_poll_msg[ALL_TX_BOARD_IDX]  = BOARD_ID();
    rx_resp_msg[ALL_RX_BOARD_IDX]  = BOARD_ID();
    tx_final_msg[ALL_TX_BOARD_IDX] = BOARD_ID();

    rx_poll_msg[ALL_RX_BOARD_IDX]  = BOARD_ID();
    tx_resp_msg[ALL_TX_BOARD_IDX]  = BOARD_ID();
    rx_final_msg[ALL_RX_BOARD_IDX] = BOARD_ID();

    /* Install DW1000 IRQ handler. */
    port_set_deca_isr(dwt_isr);

    /* Register RX call-back. */
    dwt_setcallbacks(NULL, &rx_ok_cb, NULL, NULL);

    // create msg queue for interrupt
    UwbMsgBox = osMailCreate(osMailQ(UwbMsgBox), NULL);  

    /* Enable wanted interrupts (TX confirmation, RX good frames, RX timeouts and RX errors). */
    // dwt_setinterrupt(DWT_INT_RFCG | DWT_INT_RFTO | DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFSL | DWT_INT_SFDT, 1);
    dwt_setinterrupt(DWT_INT_RFCG, 1);

    /* Set delay to turn reception on after transmission of the frame. See NOTE 2 below. */
    dwt_setrxaftertxdelay(60);

    /* Set response frame timeout. */
    dwt_setrxtimeout(0);

    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

/**
 * @brief The function gets call in an infinite loop, and consumes items from
 * a message queue that is populated by the interrupt.
 * 
 */
void uwbFrameHandler(void){

    osEvent evt;
    UwbMsg *msg_ptr;
    decaIrqStatus_t stat;


    evt = osMailGet(UwbMsgBox, osWaitForever);  // Get message on queue. 
    if (evt.status == osEventMail) {
        msg_ptr = evt.value.p;

        uint8_t msg_type = msg_ptr->msg[ALL_MSG_TYPE_IDX];

        switch (msg_type)
        {
            case 0xA:{
                stat = decamutexon(); // disable dw1000 interrupts
                twrReceiveCallback();
                decamutexoff(stat);
                break;
            }
            case 0xB:{
                usb_print("WARNING 0xB: TWR message is firing an interrupt when it shouldnt be.\r\n");
                break;
            }
            case 0xC:{
                usb_print("WARNING 0xC: TWR message is firing an interrupt when it shouldnt be.\r\n");
                break;
            }
            case 0xD:{
                dataReceiveCallback(msg_ptr->msg);
                break;
            }
            default:{
                usb_print("Unrecognized UWB message type received.");
            }
        }
        osMailFree(UwbMsgBox, msg_ptr); // IMPORTANT: free message memory
    }
}


/* MAIN RANGING FUNCTIONS ---------------------------------------- */ 
int twrInitiateInstance(uint8_t target_id, bool target_meas_bool, uint8_t mult_twr){
    int ret;
    decaIrqStatus_t stat;
    uint64 tx1_ts;
    uint64 rx2_ts, rx3_ts;

    float fpp2 = 0; // Received signal power of Signal 1 and Signal 2
    float rxp2 = 0;
    uint16_t std2 = 0;
    float skew2 = 0;

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
    dwt_setrxtimeout(0);
    
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

                ret = rxTimestampsDS(tx1_ts, rx2_ts, target_id, &fpp2, &rxp2, &std2, &skew2, 1);

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
        ret = rxTimestampsSS(0, target_id, &fpp2, &rxp2, &std2, 1);

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
                ret = txTimestampsDS(tx1_ts, rx2_ts, rx3_ts, &fpp2, &rxp2, &std2, &skew2, 1);
            }
            else{
                ret = txTimestampsSS(tx1_ts, rx2_ts, &fpp2, &rxp2, &std2, 1);
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

    float fpp1 = 0;
    float rxp1 = 0;
    uint16_t std1 = 0;
    float skew1 = 0;

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

        // If the intended target does not match the ID, passively listen on all signals.
        bool bool_target = (rx_buffer[ALL_RX_BOARD_IDX] != rx_poll_msg[ALL_RX_BOARD_IDX]);
        bool bool_msg_type = (rx_buffer[ALL_MSG_TYPE_IDX] == rx_poll_msg[ALL_MSG_TYPE_IDX]);
        if (bool_target && bool_msg_type && passive_listening){
            
            if (mult_twr){
                ret = passivelyListenDS(rx1_ts, target_meas_bool);
            }
            else{
                ret = passivelyListenSS(rx1_ts, target_meas_bool);
            }

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

            /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one.*/
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
            ret = txTimestampsDS(rx1_ts, tx2_ts, 0, &fpp1, &rxp1, &std1, &skew1, 0);
        }
        else{
            /* Transmit the delayed signal with the time-stamps */
            ret = txTimestampsSS(rx1_ts, 0, &fpp1, &rxp1, &std1, 0);
        }

        if (ret){
            /* Check if an additional signal is expected to communicate the time-stamps to the target */
            if (target_meas_bool){
                /* Set expected delay and timeout for final message reception.*/
                dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
                dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

                /* Receive the additional signal and calculate the range measurement */
                if (mult_twr){
                    ret = rxTimestampsDS(rx1_ts, tx2_ts, initiator_id, &fpp1, &rxp1, &std1, &skew1, 0);
                }
                else{
                    ret = rxTimestampsSS(rx1_ts, initiator_id, &fpp1, &rxp1, &std1, 0);
                }
                if (ret){
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

int txTimestampsSS(uint64 ts1, uint64 ts2, 
                   float* fpp, float* rxp, uint16_t* std, 
                   bool is_immediate){
    /* Set-up delayed transmission to encode the transmission time-stamp in the final message */
    int ret;
    uint8_t tx_type;

    if (is_immediate){
         /* Write all timestamps in the final message.*/
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // tx1
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL2_TS_IDX], ts2); // rx2
        
        memcpy(&tx_final_msg[FINAL_FPP_IDX], fpp, sizeof(float)); // fpp2
        memcpy(&tx_final_msg[FINAL_RXP_IDX], rxp, sizeof(float)); // rxp2
        memcpy(&tx_final_msg[FINAL_STD_IDX], std, sizeof(uint16_t)); // std2
        
        tx_type = DWT_START_TX_IMMEDIATE;
    }
    else{
        uint32 final_tx_time;
        uint64 final_tx_ts;

        /* Retrieve fpp1 */
        retrieveDiagnostics(fpp, rxp, std);

         /* Write all timestamps in the final message.*/
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // rx1
        
        memcpy(&tx_final_msg[FINAL_FPP_IDX], fpp, sizeof(float)); // fpp1
        memcpy(&tx_final_msg[FINAL_RXP_IDX], rxp, sizeof(float)); // rxp1
        memcpy(&tx_final_msg[FINAL_STD_IDX], std, sizeof(uint16_t)); // std1

        /* Compute final message transmission time. See NOTE 10 below. */
        final_tx_time = (ts1 + (1500 * UUS_TO_DWT_TIME)) >> 8;

        dwt_setdelayedtrxtime(final_tx_time);

        /* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
        final_tx_ts = (((uint64)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        /* Write timestamp in the final message.*/
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

int txTimestampsDS(uint64 ts1, uint64 ts2, uint64 ts3,
                   float* fpp, float* rxp, uint16_t* std, float* skew,
                   bool is_immediate){
    /* Set-up delayed transmission to encode the transmission time-stamp in the final message */
    int ret;
    uint8_t tx_type;

    if (is_immediate){
         /* Write all timestamps in the final message.*/
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // tx1 
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL2_TS_IDX], ts2); // rx2 
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL3_TS_IDX], ts3); // rx3
        
        memcpy(&tx_final_msg[FINAL_FPP_IDX], fpp, sizeof(float)); // fpp2
        memcpy(&tx_final_msg[FINAL_RXP_IDX], rxp, sizeof(float)); // rxp2
        memcpy(&tx_final_msg[FINAL_STD_IDX], std, sizeof(uint16_t)); // std2
        memcpy(&tx_final_msg[FINAL_SKEW_IDX], skew, sizeof(float)); // skew2
        
        tx_type = DWT_START_TX_IMMEDIATE;
    }
    else{
        uint32 final_tx_time;
        uint64 final_tx_ts;

        /* Retrieve fpp1 */
        retrieveDiagnostics(fpp, rxp, std);
        retrieveSkew(skew);

         /* Write all timestamps in the final message.*/
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL1_TS_IDX], ts1); // rx1
        final_msg_set_ts(&tx_final_msg[FINAL_SIGNAL2_TS_IDX], ts2); // tx2

        memcpy(&tx_final_msg[FINAL_FPP_IDX], fpp, sizeof(float)); // fpp1
        memcpy(&tx_final_msg[FINAL_RXP_IDX], rxp, sizeof(float)); // rxp1
        memcpy(&tx_final_msg[FINAL_STD_IDX], std, sizeof(uint16_t)); // std1
        memcpy(&tx_final_msg[FINAL_SKEW_IDX], skew, sizeof(float)); // skew1

        /* Compute final message transmission time. See NOTE 10 below. */
        final_tx_time = (ts2 + (tx3_delay * UUS_TO_DWT_TIME)) >> 8;

        dwt_setdelayedtrxtime(final_tx_time);

        /* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
        final_tx_ts = (((uint64)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        /* Write timestamp in the final message.*/
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

int rxTimestampsSS(uint64 ts1, uint8_t neighbour_id, 
                   float* fpp, float* rxp, uint16_t* std,
                   bool is_initiator){
    uint32 frame_len;
    double Ra, Db;
    uint32 rx1_ts, tx2_ts;
    uint32 tx1_ts, rx2_ts;
    int64 tof_dtu;
    char fpp1_str[10] = {0};
    char fpp2_str[10] = {0};
    char rxp1_str[10] = {0};
    char rxp2_str[10] = {0};
    float fpp1, fpp2;
    float rxp1, rxp2;
    uint16_t std1, std2;

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
                
                memcpy(&fpp1, &rx_buffer[FINAL_FPP_IDX], sizeof(float));
                memcpy(&rxp1, &rx_buffer[FINAL_RXP_IDX], sizeof(float));
                memcpy(&std1, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t));
                
                tx1_ts = (uint32)get_tx_timestamp_u64();
                rx2_ts = (uint32)get_rx_timestamp_u64();

                retrieveDiagnostics(fpp, rxp, std);
                fpp2 = *fpp;
                rxp2 = *rxp;
                std2 = *std;
            }
            else{
                /* Get timestamps embedded in the final message. */
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &tx1_ts);
                final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &rx2_ts);

                memcpy(&fpp2, &rx_buffer[FINAL_FPP_IDX], sizeof(float));
                memcpy(&rxp2, &rx_buffer[FINAL_RXP_IDX], sizeof(float));
                memcpy(&std2, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t));
            
                rx1_ts = (uint32)ts1;
                tx2_ts = (uint32)get_tx_timestamp_u64();  

                fpp1 = *fpp;  
                rxp1 = *rxp;
                std1 = *std;            
            }
            
            /* Compute time of flight. 32-bit subtractions give correct answers even if clock has wrapped. See NOTE 12 below. */            
            Ra = (double)(rx2_ts - tx1_ts);
            Db = (double)(tx2_ts - rx1_ts);
            tof_dtu = (int64)((Ra - Db) / (2)); // Standard single-sided TWR
            
            tof = tof_dtu * DWT_TIME_UNITS;
            distance = tof * SPEED_OF_LIGHT;

            convert_float_to_string(fpp1_str,fpp1);
            convert_float_to_string(fpp2_str,fpp2);
            convert_float_to_string(rxp1_str,rxp1);
            convert_float_to_string(rxp2_str,rxp2);

            /* Display computed distance. */
            char dist_str[10] = {0};
            convert_float_to_string(dist_str,distance);
            char response[100];
            
            char* prefix[4];
            if (is_initiator){
                *prefix = "R05";
            }
            else{
                *prefix = "S05";
            }

            sprintf(response, "%s|%d|%s|%lu|%lu|%lu|%lu|0|0|%s|%s|%s|%s|%u|%u\r\n",
                    *prefix,
                    neighbour_id, dist_str,
                    tx1_ts, rx1_ts,
                    tx2_ts, rx2_ts,
                    fpp1_str, fpp2_str,
                    rxp1_str, rxp2_str,
                    std1, std2);
            usb_print(response);
            
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

int rxTimestampsDS(uint64 ts1, uint64 ts2, uint8_t neighbour_id, 
                   float* fpp, float* rxp, uint16_t* std, float* skew,
                   bool is_initiator){
    uint32 frame_len;
    double Ra1, Ra2, Db1, Db2;
    uint32 rx1_ts, tx2_ts, tx3_ts;
    uint32 tx1_ts, rx2_ts, rx3_ts;
    double tof_dtu;
    char fpp1_str[10] = {0};
    char fpp2_str[10] = {0};
    char rxp1_str[10] = {0};
    char rxp2_str[10] = {0};
    char skew1_str[10] = {0};
    char skew2_str[10] = {0};
    float fpp1, fpp2;
    float rxp1, rxp2;
    uint16_t std1, std2;
    float skew1, skew2;
    
    retrieveDiagnostics(fpp, rxp, std);
    retrieveSkew(skew);

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

                memcpy(&fpp1, &rx_buffer[FINAL_FPP_IDX], sizeof(float));
                memcpy(&rxp1, &rx_buffer[FINAL_RXP_IDX], sizeof(float));
                memcpy(&std1, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t));
                memcpy(&skew1, &rx_buffer[FINAL_SKEW_IDX], sizeof(float));

                fpp2 = *fpp;
                rxp2 = *rxp;
                std2 = *std;
                skew2 = *skew;

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

                memcpy(&fpp2, &rx_buffer[FINAL_FPP_IDX], sizeof(float));
                memcpy(&rxp2, &rx_buffer[FINAL_RXP_IDX], sizeof(float));
                memcpy(&std2, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t));
                memcpy(&skew2, &rx_buffer[FINAL_SKEW_IDX], sizeof(float));

                fpp1 = *fpp;
                rxp1 = *rxp;
                std1 = *std;
                skew1 = *skew;

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
            // tof_dtu = (int64)((Ra1*Db2 - Ra2*Db1) / (Ra2 + Db2)); // Reversed alternative double-sided TWR
            tof_dtu = (double)(0.5*(Ra1 - Ra2/Db2*Db1)); // Reversed alternative double-sided TWR
           
            tof = tof_dtu * DWT_TIME_UNITS;
            distance = tof * SPEED_OF_LIGHT;

            /* Display computed distance. */
            char dist_str[10] = {0};
            convert_float_to_string(dist_str,distance);
            char response[150];

            char* prefix[4];
            if (is_initiator){
                *prefix = "R05";
            }
            else{
                *prefix = "S05";
            }

            convert_float_to_string(fpp1_str,fpp1);
            convert_float_to_string(fpp2_str,fpp2);
            convert_float_to_string(rxp1_str,rxp1);
            convert_float_to_string(rxp2_str,rxp2);
            convert_float_to_string(skew1_str,skew1);
            convert_float_to_string(skew2_str,skew2);

            sprintf(response, "%s|%d|%s|%lu|%lu|%lu|%lu|%lu|%lu|%s|%s|%s|%s|%u|%u|%s|%s\r\n",
                    *prefix,
                    neighbour_id, dist_str,
                    tx1_ts, rx1_ts,
                    tx2_ts, rx2_ts,
                    tx3_ts, rx3_ts,
                    fpp1_str, fpp2_str,
                    rxp1_str, rxp2_str,
                    std1, std2,
                    skew1_str, skew2_str);
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

/*! ----------------------------------------------------------------------------
 * Function: passivelyListenSS()
 *
 * @brief Listen passively to other TWR tags and record all timestamps. 
 * Single-sided version.
 * 
 * @param rx_ts1 (uint32_t) The time of reception of the already detected signal 
 * through interrupt.
 * @param target_meas_bool (bool) Whether or not a third signal is expected.
 * 
 * @return (bool) Success boolean.
 */
int passivelyListenSS(uint32_t rx_ts1, bool target_meas_bool){
    /* Have received first signal in a TWR transaction. 1 or 2 more signals expected. */
    bool success;
    uint8_t initiator_id, target_id;
    uint32_t rx_ts2 = 0; // reception timestamps at current tag
    uint32_t tx_ts1_n = 0, tx_ts2_n = 0; // transmission timestamps at neighbouring tags
    uint32_t rx_ts1_n = 0, rx_ts2_n = 0; // reception timestamps at neighbouring tags
    float fpp1 = 0, fpp2 = 0; // received signal power at current tag 
    float rxp1 = 0, rxp2 = 0;  
    uint16_t std1 = 0, std2 = 0;  
    float fpp1_n = 0, fpp2_n = 0; // received signal power at neighbouring tags 
    float rxp1_n = 0, rxp2_n = 0;
    uint16_t std1_n = 0, std2_n = 0;
    char fpp1_str[10] = {0};
    char fpp2_str[10] = {0};
    char rxp1_str[10] = {0};
    char rxp2_str[10] = {0};
    char fpp1_n_str[10] = {0};
    char fpp2_n_str[10] = {0};
    char rxp1_n_str[10] = {0};
    char rxp2_n_str[10] = {0};

    // Retrieve IDs of tags involved in the TWR transaction
    initiator_id = rx_buffer[ALL_TX_BOARD_IDX];
    target_id = rx_buffer[ALL_RX_BOARD_IDX];

    /* Retrieve received signal power */
    retrieveDiagnostics(&fpp1, &rxp1, &std1);

    /* --------------------- SIGNAL 2: Target to Initiator --------------------- */
    success = checkReceivedFrame(ALL_RX_BOARD_IDX, initiator_id, ALL_TX_BOARD_IDX, target_id, 0xC);
    if (success){
        /* Extract all the embedded information in the received signal */
        final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &rx_ts1_n);
        final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &tx_ts2_n);

        memcpy(&fpp1_n, &rx_buffer[FINAL_FPP_IDX], sizeof(float)); 
        memcpy(&rxp1_n, &rx_buffer[FINAL_RXP_IDX], sizeof(float)); 
        memcpy(&std1_n, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t)); 

        /* Retrieve reception timestamp */
        rx_ts2 = get_rx_timestamp_u64();

        /* Retrieve received signal power */
        retrieveDiagnostics(&fpp2, &rxp2, &std2);
    }
    else{
        return 0;
    }

    /* --------------------- SIGNAL 3: Initiator to Target --------------------- 
    The transmission time-stamp of Signal 1 is embedded in the received frame */
    // Check if fourth signal is expected.
    if (target_meas_bool){
        dwt_setpreambledetecttimeout(0);
        dwt_setrxtimeout(0);
        success = checkReceivedFrame(ALL_TX_BOARD_IDX, initiator_id, ALL_RX_BOARD_IDX, target_id, 0xC);
        if (success){
            /* Extract all the embedded information in the received signal */
            final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &tx_ts1_n);
            final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &rx_ts2_n);

            memcpy(&fpp2_n, &rx_buffer[FINAL_FPP_IDX], sizeof(float)); 
            memcpy(&rxp2_n, &rx_buffer[FINAL_RXP_IDX], sizeof(float)); 
            memcpy(&std2_n, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t)); 
        }
        else{
            return 0;
        }
    }

    /* --------------------- Output Time-stamps --------------------- */
    convert_float_to_string(fpp1_str,fpp1);
    convert_float_to_string(fpp2_str,fpp2);
    convert_float_to_string(rxp1_str,rxp1);
    convert_float_to_string(rxp2_str,rxp2);
    convert_float_to_string(fpp1_n_str,fpp1_n);
    convert_float_to_string(fpp2_n_str,fpp2_n);
    convert_float_to_string(rxp1_n_str,rxp1_n);
    convert_float_to_string(rxp2_n_str,rxp2_n);

    char output[200];
    sprintf(output,"S01|%d|%d|%lu|%lu|0|%lu|%lu|%lu|%lu|0|0|%s|%s|0|%s|%s|0|%u|%u|0|%s|%s|%s|%s|%u|%u\r\n",
            initiator_id, target_id,
            rx_ts1,rx_ts2,
            tx_ts1_n,rx_ts1_n,
            tx_ts2_n,rx_ts2_n,
            fpp1_str,fpp2_str,
            rxp1_str,rxp2_str,
            std1,std2,
            fpp1_n_str,fpp2_n_str,
            rxp1_n_str,rxp2_n_str,
            std1_n,std2_n);
    usb_print(output);
    return 1;
}

/*! ----------------------------------------------------------------------------
 * Function: passivelyListenDS()
 *
 * @brief Listen passively to other TWR tags and record all timestamps. 
 * Double-sided version.
 * 
 * @param rx_ts1 (uint32_t) The time of reception of the already detected signal 
 * through interrupt.
 * @param target_meas_bool (bool) Whether or not a fourth signal is expected.
 * 
 * @return (bool) Success boolean.
 */
int passivelyListenDS(uint32_t rx_ts1, bool target_meas_bool){
    /* Have received first signal in a TWR transaction. 1 or 2 more signals expected. */
    bool success;
    uint8_t initiator_id, target_id;
    uint32_t rx_ts2 = 0, rx_ts3 = 0; // reception timestamps at current tag
    uint32_t tx_ts1_n = 0, tx_ts2_n = 0, tx_ts3_n = 0; // transmission timestamps at neighbouring tags
    uint32_t rx_ts1_n = 0, rx_ts2_n = 0, rx_ts3_n = 0; // reception timestamps at neighbouring tags
    float fpp1 = 0, fpp2 = 0, fpp3 = 0; // received signal power at current tag 
    float rxp1 = 0, rxp2 = 0, rxp3 = 0;
    uint16_t std1 = 0, std2 = 0, std3 = 0;
    float fpp1_n = 0, fpp2_n = 0; // received signal power at neighbouring tags 
    float rxp1_n = 0, rxp2_n = 0;
    uint16_t std1_n = 0, std2_n = 0;
    char fpp1_str[10] = {0};
    char fpp2_str[10] = {0};
    char fpp3_str[10] = {0};
    char rxp1_str[10] = {0};
    char rxp2_str[10] = {0};
    char rxp3_str[10] = {0};
    char fpp1_n_str[10] = {0};
    char fpp2_n_str[10] = {0};
    char rxp1_n_str[10] = {0};
    char rxp2_n_str[10] = {0};

    // Retrieve IDs of tags involved in the TWR transaction
    initiator_id = rx_buffer[ALL_TX_BOARD_IDX];
    target_id = rx_buffer[ALL_RX_BOARD_IDX];

    /* Retrieve received signal power */
    retrieveDiagnostics(&fpp1, &rxp1, &std1);

    /* --------------------- SIGNAL 2: Target to Initiator --------------------- */
    success = checkReceivedFrame(ALL_RX_BOARD_IDX, initiator_id, ALL_TX_BOARD_IDX, target_id, 0xB);
    if (success){
        /* Retrieve reception timestamp */
        rx_ts2 = get_rx_timestamp_u64();

        /* Retrieve received signal power */
        retrieveDiagnostics(&fpp2, &rxp2, &std2);
    }
    else{
        /* Due to immediate response of Signal 2, this has highest chance of failure.
           If failed, still communicate the ranging tags' IDs for scheduling purposes. */
        char output[155];
        sprintf(output,"S01|%d|%d|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0\r\n",
                initiator_id,target_id);
        usb_print(output);
        return 0;
    }

    /* --------------------- SIGNAL 3: Target to Initiator --------------------- */
    success = checkReceivedFrame(ALL_RX_BOARD_IDX, initiator_id, ALL_TX_BOARD_IDX, target_id, 0xC);
    if (success){
        /* Extract all the embedded information in the received signal */
        final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &rx_ts1_n);
        final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &tx_ts2_n);
        final_msg_get_ts(&rx_buffer[FINAL_SIGNAL3_TS_IDX], &tx_ts3_n);

        memcpy(&fpp1_n, &rx_buffer[FINAL_FPP_IDX], sizeof(float)); 
        memcpy(&rxp1_n, &rx_buffer[FINAL_RXP_IDX], sizeof(float)); 
        memcpy(&std1_n, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t)); 

        /* Retrieve reception timestamp */
        rx_ts3 = get_rx_timestamp_u64();

        /* Retrieve received signal power */
        retrieveDiagnostics(&fpp3, &rxp3, &std3);
    }
    else{
        return 0;
    }

    /* --------------------- SIGNAL 4: Initiator to Target --------------------- 
    The transmission time-stamp of Signal 1 is embedded in the received frame */
    // Check if fourth signal is expected.
    if (target_meas_bool){
        dwt_setpreambledetecttimeout(0);
        dwt_setrxtimeout(0);

        success = checkReceivedFrame(ALL_TX_BOARD_IDX, initiator_id, ALL_RX_BOARD_IDX, target_id, 0xC);
        if (success){
            /* Extract all the embedded information in the received signal */
            final_msg_get_ts(&rx_buffer[FINAL_SIGNAL1_TS_IDX], &tx_ts1_n);
            final_msg_get_ts(&rx_buffer[FINAL_SIGNAL2_TS_IDX], &rx_ts2_n);
            final_msg_get_ts(&rx_buffer[FINAL_SIGNAL3_TS_IDX], &rx_ts3_n);

            memcpy(&fpp2_n, &rx_buffer[FINAL_FPP_IDX], sizeof(float)); 
            memcpy(&rxp2_n, &rx_buffer[FINAL_RXP_IDX], sizeof(float)); 
            memcpy(&std2_n, &rx_buffer[FINAL_STD_IDX], sizeof(uint16_t)); 
        }
        else{
            return 0;
        }
    }

    /* --------------------- Output Time-stamps --------------------- */
    convert_float_to_string(fpp1_str,fpp1);
    convert_float_to_string(fpp2_str,fpp2);
    convert_float_to_string(fpp3_str,fpp3);
    convert_float_to_string(rxp1_str,rxp1);
    convert_float_to_string(rxp2_str,rxp2);
    convert_float_to_string(rxp3_str,rxp3);
    convert_float_to_string(fpp1_n_str,fpp1_n);
    convert_float_to_string(fpp2_n_str,fpp2_n);
    convert_float_to_string(rxp1_n_str,rxp1_n);
    convert_float_to_string(rxp2_n_str,rxp2_n);

    char output[200];
    sprintf(output,"S01|%d|%d|%lu|%lu|%lu|%lu|%lu|%lu|%lu|%lu|%lu|%s|%s|%s|%s|%s|%s|%u|%u|%u|%s|%s|%s|%s|%u|%u\r\n",
            initiator_id, target_id,
            rx_ts1,rx_ts2,rx_ts3,
            tx_ts1_n,rx_ts1_n,
            tx_ts2_n,rx_ts2_n,
            tx_ts3_n,rx_ts3_n,
            fpp1_str,fpp2_str,fpp3_str,
            rxp1_str,rxp2_str,rxp3_str,
            std1,std2,std3,
            fpp1_n_str,fpp2_n_str,
            rxp1_n_str,rxp2_n_str,
            std1_n,std2_n);
    usb_print(output);
    return 1;
}

/*! ----------------------------------------------------------------------------
 * Function: checkReceivedFrame()
 *
 * @brief This function checks if the received frame is the expected one. 
 * 
 * @param initator_idx (uint8_t) The index of the initator ID in the received message.
 * @param initator_id (uint8_t) The ID of the initator board in this TWR transaction. 
 * @param target_idx (uint8_t) The index of the target ID in the received message.
 * @param target_id (uint8_t) The ID of the target board in this TWR transaction.
 * @param msg_type (uint8_t) The type of message expected.
 * 
 * @return (bool) Success boolean.
 */
bool checkReceivedFrame(uint8_t initiator_idx, uint8_t initiator_id,
                        uint8_t target_idx, uint8_t target_id,
                        uint8_t msg_type){
    uint32 frame_len;
    bool bool_type, bool_initiator, bool_target;
    
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
    
        /* Check that the frame is the expected response. */
        bool_type = rx_buffer[ALL_MSG_TYPE_IDX] == msg_type;
        bool_initiator = rx_buffer[initiator_idx] == initiator_id;
        bool_target = rx_buffer[target_idx] == target_id;
        if (bool_type && bool_initiator && bool_target){
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

/*! ----------------------------------------------------------------------------
 * Function: setPassiveToggle()
 * 
 * @brief This function sets the passive toggle.
 * 
 * @param toggle (bool) 1 if passive toggle is to be turned on, 0 otherwise.
 */
void setPassiveToggle(bool toggle){
    passive_listening = toggle;
}

/*! ----------------------------------------------------------------------------
 * Function: setResponseDelay()
 * 
 * @brief This function sets the tx3 response delay.
 * 
 * @param delay (uint16) the delay in microseconds between tx2 and tx3.
 */
void setResponseDelay(uint16 delay){
    tx3_delay = delay;
}

/*! ----------------------------------------------------------------------------
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
    
    /* Clear local RX buffer to avoid having leftovers from previous receptions. 
    This is not necessary but is included here to aid reading the RX
    buffer. */  
    int i;
    for (i = 0 ; i < MAX_FRAME_LEN; i++ )
    {
        rx_buffer[i] = 0;
    }

    /* A frame has been received, copy it to our local buffer. */
    if (cb_data->datalength <= MAX_FRAME_LEN)
    {
        // Load data directly into a static buffer.
        dwt_readrxdata(rx_buffer, cb_data->datalength, 0);

        // Allocate memory for message struct 
        UwbMsg *msg_ptr;
        msg_ptr = osMailCAlloc(UwbMsgBox, 0);   // Allocate memory for the Mail

        // Load data into the message 
        msg_ptr->len = cb_data->datalength;
        dwt_readrxdata(msg_ptr->msg, cb_data->datalength, 0);

        // Send message to the queue
        osMailPut(UwbMsgBox, msg_ptr);

        //CDC_Transmit_FS(rx_buffer, cb_data->datalength); // for debugging
    }
}

