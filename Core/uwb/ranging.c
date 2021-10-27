/**
  ******************************************************************************
  * @file    ranging.c
  * @brief   This file provides code for UWB ranging, such as one-way
  *          ranging (OWR) and two-way ranging (TWR).
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ranging.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "spi.h"
#include "stm32f4xx_hal_conf.h"
#include "main.h"
#include "usb.h"

#include "cmsis_os.h"

/* Default communication configuration. 
In Decawave's examples, the default mode (mode 3) is used. 
We use here Justin Cano's settings, for the time being. */
static dwt_config_t config = {
		2,               /* Channel number. */
		DWT_PRF_64M,     /* Pulse repetition frequency. */
		DWT_PLEN_128,   /* Preamble length. Used in TX only. */
		DWT_PAC8,       /* Preamble acquisition chunk size. Used in RX only. */
		9,               /* TX preamble code. Used in TX only. */
		9,               /* RX preamble code. Used in RX only. */
		0,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
		DWT_BR_6M8,     /* Data rate. */
		DWT_PHRMODE_STD, /* PHY header mode. */
		(129 + 8 - 8) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 1000

/* Buffer to store received frame. See NOTE 1 below. */
#define FRAME_LEN_MAX 127
static uint8 rx_buffer[FRAME_LEN_MAX];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

static uint8 tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};

int do_owr(void){
    int result;
    
    dwt_setrxaftertxdelay(40); //POLL_TX_TO_RESP_RX_DLY_UUS);
	dwt_setrxtimeout(800);//RESP_RX_TIMEOUT); // Prepare for rx_resp

    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

    /* Write frame data to DW1000 and prepare transmission. See NOTE 4 below.*/
    dwt_writetxdata(sizeof(tx_msg), tx_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_msg), 0, 1); /* Zero offset in TX buffer, no ranging. */

    /* Start transmission. */
    result = dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

    // /* Poll DW1000 until TX frame sent event set. See NOTE 5 below.
    //     * STATUS register is 5 bytes long but, as the event we are looking at is in the first byte of the register, we can use this simplest API
    //     * function to access it.*/
    // while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
    // { };

    // /* Clear TX frame sent event. */
    // dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

    // /* Execute a delay between transmissions. */
    // deca_sleep(TX_DELAY_MS);

    if(result==DWT_SUCCESS){

		return 1; // Msg correctly sent
	}
	else{
		return 0; // Error
	}
}

void listen(){
    uint32_t attempts = 0;
    int i;

    /* TESTING BREAKPOINT LOCATION #1 */

    /* Clear local RX buffer to avoid having leftovers from previous receptions  This is not necessary but is included here to aid reading
        * the RX buffer.
        * This is a good place to put a breakpoint. Here (after first time through the loop) the local status register will be set for last event
        * and if a good receive has happened the data buffer will have the data in it, and frame_len will be set to the length of the RX frame. */
    for (i = 0 ; i < FRAME_LEN_MAX; i++ )
    {
        rx_buffer[i] = 0;
    }

    dwt_setrxtimeout(2000*UUS_TO_DWT_TIME);

    /* Activate reception immediately. See NOTE 3 below. */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    /* Poll until a frame is properly received or an error/timeout occurs. See NOTE 4 below.
        * STATUS register is 5 bytes long but, as the event we are looking at is in the first byte of the register, we can use this simplest API
        * function to access it. */
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    {
		attempts++;
		if(attempts>1000){ // problem reset
			return -1; // Complete failure : reboot
		}
	};

    if (status_reg & SYS_STATUS_RXFCG)
    {
        /* Hold copy of frame length of frame received (if good) so that it can be examined at a debug breakpoint. */
        uint32 frame_len;

        /* Clear good RX frame event in the DW1000 status register. */
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

        /* A frame has been received, copy it to our local buffer. */
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
        if (frame_len <= FRAME_LEN_MAX)
        {
            dwt_readrxdata(rx_buffer, frame_len, 0);

            if (memcmp(rx_buffer, tx_msg, 1) == 0)
            {
                usb_print("Frame received!\r\n");
                //Good candidate
                usb_print(rx_buffer);
                usb_print(" \n");
            }
        }       
    }
}

void uwb_init(void){
     /* Reset and initialise DW1000.
     * For initialisation, DW1000 clocks must be temporarily set to crystal speed. After initialisation SPI rate can be increased for optimum
     * performance. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
    {
        usb_print("UWB initialization failed. \n");
        while (1)
        { };
    }
    port_set_dw1000_fastrate();

    /* Configure DW1000. */
    dwt_configure(&config);

    /* Apply default antenna delay value. See NOTE 1 below. */
	dwt_setrxantennadelay(RX_ANT_DLY);
	dwt_settxantennadelay(TX_ANT_DLY);

    usb_print("UWB tag initialized and configured. \n");
}

/* @fn      reset_DW1000
 * @brief   DW_RESET pin on DW1000 has 2 functions
 *          In general it is output, but it also can be used to reset the digital
 *          part of DW1000 by driving this pin low.
 *          Note, the DW_RESET pin should not be driven high externally.
 * */
void reset_DW1000(void)
{
    GPIO_InitTypeDef    GPIO_InitStruct;

    // Enable GPIO used for DW1000 reset as open collector output
    GPIO_InitStruct.Pin = DW_RESET_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);

    //drive the RSTn pin low
    HAL_GPIO_WritePin(DW_RESET_GPIO_Port, DW_RESET_Pin, GPIO_PIN_RESET);

    //put the pin back to tri-state ... as input
	GPIO_InitStruct.Pin = DW_RESET_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);

    osDelay(2);
}