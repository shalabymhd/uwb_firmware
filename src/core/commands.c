/**
  ******************************************************************************
  * @file    commands.c
  * @brief   The main file for all the USB command implementations. All commands
  *          functions must have the same inputs and outputs.
  * 
    NOTE: currently ALL commands are endlessly retried until they return a 
    value of 1, or until a new command sent over USB overwrites. This might not 
    be the desired behavior for some future functions, where they might just 
    want to report a failure and not retry. 

    A simple solution is to extend the possible return values of the commands:
    -1: Fail, retry me.
    0: Fail, 
    1: success
    
  ******************************************************************************
  */

#include "commands.h"
#include "common.h"
#include "ranging.h"
#include "uthash.h"
#include "main.h" 
#include <stdbool.h>
#include "messaging.h"
#include "common.h"
#include "usbd_cdc_if.h"
#include <stdlib.h>
#include "usb_device.h"
#include "spi.h"

int c00_set_idle(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    usb_print("R00\r\n");
    osDelay(1);
    return 1;
}

int c01_get_id(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    char id_str[10];
    sprintf(id_str, "R01|%u\r\n", BOARD_ID()); 
    usb_print(id_str);
    osDelay(1);
    return 1;
}

int c02_reset(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    // Reset the UWB receiver. TODO: would we need to reset other things? Could even do a hard reset.
    dwt_rxreset();

    // Turn on the UWB receiver
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    
    usb_print("R02\r\n");
    return 1;
}

int c03_do_tests(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    /* TODO: 
    1) Add any other tests necessary.
    2) When we add tests, it might be worthwhile to give different 
       errors an ID and just output the error ID.

    */
    IntParams *i;
    BoolParams *b; 
    StrParams *s;
    FloatParams *f;
    ByteParams *y;
    char float_as_string[10] = {0};
    char response[300] = {0};

    HASH_FIND_STR(msg_ints, "test_int", i);
    HASH_FIND_STR(msg_strs, "test_str", s);
    HASH_FIND_STR(msg_bools, "test_bool", b);
    HASH_FIND_STR(msg_floats, "test_flt", f);
    HASH_FIND_STR(msg_bytes, "test_byte", y);

    uint8_t my_id;
    uint8_t error_id = 0;
    dwt_geteui(&my_id);
    if(my_id != BOARD_ID()){
        error_id = 1;
    }
    convert_float_to_string(float_as_string, f->value);

    sprintf(response,"R03|%u|%d|%s|%d|%s|", error_id, i->value, s->value, b->value, float_as_string);
    uint16_t len = strlen(response);

    memcpy(&response[len], &(y->len), 2);
    len += 2;
    memcpy(&response[len], y->value, y->len);
    len += y->len;
    memcpy(&response[len],"\r\n", 2);
    len += 2;
    CDC_Transmit_FS((u_int8_t *) &response[0], len);
   
    return 1;
}

int c04_toggle_passive(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    BoolParams *i;
    
    /* Extract the toggle */
    HASH_FIND_STR(msg_bools, "toggle", i);
    
    setPassiveToggle(i->value);

    usb_print("R04\r\n");
    
    return 1;
}

int c05_initiate_twr(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    bool success;
    uint8_t target_ID, mult_twr;
    IntParams *i;
    bool target_meas_bool;
    BoolParams *b;

    /* Extract the target */
    HASH_FIND_STR(msg_ints, "target", i);
    target_ID = i->value;

    /* Extract the toggle that dictates if the target computes range measurements */
    HASH_FIND_STR(msg_bools, "targ_meas", b);
    target_meas_bool = b->value;

    /* Extract the toggle that dictates if the multiplicative TWR will be used */
    HASH_FIND_STR(msg_ints, "mult_twr", i);
    mult_twr = i->value;

    if (target_ID == BOARD_ID()){
        usb_print("TWR FAIL: The target ID is the same as the initiator's ID.\r\n");
        return 0;
        // TODO: we should not retry!! we will get stuck.
    }

    success = twrInitiateInstance(target_ID, target_meas_bool, mult_twr);

    if (success){ 
        // Response is done inside `twrInitiateInstance`
        usb_print("TWR SUCCESS!\r\n");
        return 1;
    }
    else {
        // TODO: this is worth retrying. need to implement a limit.
        usb_print("TWR FAIL: No successful response.\r\n");
        return 0;
    }
}


int c06_broadcast(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    
    ByteParams *b;
    
    HASH_FIND_STR(msg_bytes, "data", b);
    uint8_t *msg = &(b->value[0]);

    // TODO: we need to standardize the response when success/fail.
    bool success;
    success = broadcast(msg, b->len);
    osDelay(1);
    if (success){
        usb_print("R06\r\n");
        return 1;
    }
    else {
        return 0;
    }
    osDelay(1);
}

int c07_get_max_frame_len(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    char response[20];
    sprintf(response, "R07|%u\r\n", MAX_FRAME_LEN); 
    usb_print(response);
    return 1;
}

/* ************************************************************************** */
/**
 * @brief Jump to the bootloader system memory from software. This will put the
 * board in a bootloading state, ready to accept new firmware from USB. 
 * 
 * This function was taken from 
 * https://stm32f4-discovery.net/2017/04/tutorial-jump-system-memory-software-stm32/
 * 
 */
int c08_jump_to_bootloader(IntParams *msg_ints, FloatParams *msg_floats, BoolParams *msg_bools, StrParams *msg_strs, ByteParams *msg_bytes){
    
    
    usb_print("R08\r\n");
    osDelay(100);
    jump_to_bootloader();
    return 0; // should never get here.
}

void jump_to_bootloader(void){

    
    /**
     * Step: Set system memory address.
     *
     *       For STM32F429, system memory is on 0x1FFF 0000
     *       For other families, check AN2606 document table 159 with descriptions of memory addresses
     */
    volatile uint32_t addr = 0x1FFF0000;

    /**
     * Step: Disable RCC, set it to default (after reset) settings
     *       Internal clock, no PLL, etc.
     */
    decamutexon();
    reset_DW1000();
    SPI1_DeInit(); // Disable SPI
    HAL_NVIC_DisableIRQ(DECAIRQ_EXTI_IRQn); // Disable decawave interrupt.
    USB_DEVICE_DeInit(); // Disable USB
#if defined(USE_HAL_DRIVER)
    HAL_RCC_DeInit();
    HAL_DeInit(); // add by ctien
#endif /* defined(USE_HAL_DRIVER) */
#if defined(USE_STDPERIPH_DRIVER)
    RCC_DeInit();
#endif /* defined(USE_STDPERIPH_DRIVER) */

    /**
     * Step: Disable systick timer and reset it to default values
     */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    /**
     * Step: Disable all interrupts
     * Charles Cossette: ACTUALLY, this will stop USB from working. 
     * so we cannot disable all interrupts. 
     */
    //__disable_irq(); // changed by ctien

    /**
     * Step: Remap system memory to address 0x0000 0000 in address space
     *       For each family registers may be different.
     *       Check reference manual for each family.
     *
     *       For STM32F4xx, MEMRMP register in SYSCFG is used (bits[1:0])
     *       For STM32F0xx, CFGR1 register in SYSCFG is used (bits[1:0])
     *       For others, check family reference manual
     */
    //Remap by hand... {
// #if defined(STM32F4)
//    SYSCFG->MEMRMP = 0x01;
// #endif
// #if defined(STM32F0)
//     SYSCFG->CFGR1 = 0x01;
// #endif
    //} ...or if you use HAL drivers
    __HAL_RCC_SYSCFG_CLK_ENABLE(); //make sure syscfg clocked
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();    //Call HAL macro to do this for you
    SCB->VTOR = 0; //set vector table offset to 0
    
    /**
     * Step: Set main stack pointer.
     *       This step must be done last otherwise local variables in this function
     *       don't have proper value since stack pointer is located on different position
     *
     *       Set direct address location which specifies stack pointer in SRAM location
     */
    __set_MSP(*(uint32_t *)addr);


    void (*SysMemBootJump)(void);
    /**
     * Step: Set jump memory location for system memory
     *       Use address with 4 bytes offset which specifies jump location where program starts
     */
    SysMemBootJump = (void (*)(void)) (*((uint32_t *)(0x1FFF0004)));


    /**
     * Step: Actually call our function to jump to set location
     *       This will start system memory execution
     */
    
    SysMemBootJump();

    
    /**
     * Step: Connect USB cable to computer and flash using either STM32 Cube 
     * Programmer or with dfu-util using 
     * 
     * dfu-util -a 0 --dfuse-address 0x08000000:leave -D ./build/firmware.bin 
     */


    // Should never get here. blink at high frequency if you did.
    while (1){
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
        HAL_Delay(50);
    }
}