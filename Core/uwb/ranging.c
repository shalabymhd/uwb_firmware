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
    DWT_PLEN_1024,   /* Preamble length. Used in TX only. */
    DWT_PAC32,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_110K,     /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

void do_owr(void){
    uwb_init();
}

void uwb_init(void){
     /* Reset and initialise DW1000.
     * For initialisation, DW1000 clocks must be temporarily set to crystal speed. After initialisation SPI rate can be increased for optimum
     * performance. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
    {
        usb_print("UWB initialization failed. \n");
        while (1)
        { };
    }
    port_set_dw1000_fastrate();

    /* Configure DW1000. */
    dwt_configure(&config);

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
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);

    //drive the RSTn pin low
    HAL_GPIO_WritePin(DW_RESET_GPIO_Port, DW_RESET_Pin, GPIO_PIN_RESET);

    osDelay(1);

    //put the pin back to tri-state ... as input
	GPIO_InitStruct.Pin = DW_RESET_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(DW_RESET_GPIO_Port, &GPIO_InitStruct);

    osDelay(2);
}