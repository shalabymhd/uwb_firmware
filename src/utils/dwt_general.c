/**
 * @brief  Initializes DWT_Clock_Cycle_Count for DWT_Delay_us, getInterval functions
 * @return Error DWT counter
 *         1: clock cycle counter not started
 *         0: clock cycle counter works
 * 
 * Extracted from:
 * https://deepbluembedded.com/stm32-delay-microsecond-millisecond-utility-dwt-delay-timer-delay/
 *
 */

#include "dwt_general.h"
#include "main.h"
#include "common.h"

/* DW1000 IRQ handler declaration. */
    port_deca_isr_t port_deca_isr;

uint32_t DWT_Delay_Init(void) {
    /* Disable TRC */
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
    /* Enable TRC */
    CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;

    /* Disable clock cycle counter */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
    /* Enable  clock cycle counter */
    DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

    /* Reset the clock cycle counter value */
    DWT->CYCCNT = 0;

    /* 3 NO OPERATION instructions */
    __ASM volatile ("NOP");
    __ASM volatile ("NOP");
    __ASM volatile ("NOP");

    /* Check if clock cycle counter has started */
    if (DWT->CYCCNT) {
        return 0; /*clock cycle counter started*/
    }
    else {
        return 1; /*clock cycle counter not started*/
    }
}

/* Returns the interval between previous_ticks and current ticks. */
uint32_t getInterval(uint32_t* previous_ticks_buff){
	uint32_t ret, ticks, previous_ticks;
	previous_ticks = previous_ticks_buff[0]; // Unstacking of the buffer
	ticks = DWT->CYCCNT; // Lecture of the current number of ticks
	int64_t interval = ticks - previous_ticks; // Computation of the interval (precaution with bit extension)
	if(interval<0){ // If the interval is negative take countermeasure
		 ret = (0xFFFFFFFF)-previous_ticks; // Complement with otherflow
		 ret+= ticks; // Addition of the interval
	}
	else{
		// Normal case convert to a simple long
		ret = (uint32_t) interval;
	}
	previous_ticks_buff[0]=ticks; // Restacking of the buffer
	ret /= (HAL_RCC_GetHCLKFreq() / 1000000); // Conversion of the interval to microseconds
	return ret; // return the corrected interval
}

/* @fn      port_DisableEXT_IRQ
 * @brief   wrapper to disable DW_IRQ pin IRQ
 *          in current implementation it disables all IRQ from lines 5:9
 * */
__INLINE void port_DisableEXT_IRQ(void)
{
    NVIC_DisableIRQ(DECAIRQ_EXTI_IRQn);
}

/* @fn      port_EnableEXT_IRQ
 * @brief   wrapper to enable DW_IRQ pin IRQ
 *          in current implementation it enables all IRQ from lines 5:9
 * */
__INLINE void port_EnableEXT_IRQ(void)
{
    NVIC_EnableIRQ(DECAIRQ_EXTI_IRQn);
}

/* @fn      port_GetEXT_IRQStatus
 * @brief   wrapper to read a DW_IRQ pin IRQ status
 * */
__INLINE uint32_t port_GetEXT_IRQStatus(void)
{
    return EXTI_GetITEnStatus(DECAIRQ_EXTI_IRQn);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: decamutexon()
 *
 * Description: This function should disable interrupts. This is called at the start of a critical section
 * It returns the irq state before disable, this value is used to re-enable in decamutexoff call
 *
 * Note: The body of this function is defined in deca_mutex.c and is platform specific
 *
 * input parameters:	
 *
 * output parameters
 *
 * returns the state of the DW1000 interrupt
 */
decaIrqStatus_t decamutexon(void)           
{
	decaIrqStatus_t s = port_GetEXT_IRQStatus();

	if(s) {
		port_DisableEXT_IRQ(); //disable the external interrupt line
	}
	return s ;   // return state before disable, value is used to re-enable in decamutexoff call
}

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: decamutexoff()
 *
 * Description: This function should re-enable interrupts, or at least restore their state as returned(&saved) by decamutexon 
 * This is called at the end of a critical section
 *
 * Note: The body of this function is defined in deca_mutex.c and is platform specific
 *
 * input parameters:	
 * @param s - the state of the DW1000 interrupt as returned by decamutexon
 *
 * output parameters
 *
 * returns the state of the DW1000 interrupt
 */
void decamutexoff(decaIrqStatus_t s)        // put a function here that re-enables the interrupt at the end of the critical section
{
	if(s) { //need to check the port state as we can't use level sensitive interrupt on the STM ARM
		port_EnableEXT_IRQ();
	}
}

void port_set_deca_isr(port_deca_isr_t deca_isr)
{
    /* Check DW1000 IRQ activation status. */
    ITStatus en = port_GetEXT_IRQStatus();

    /* If needed, deactivate DW1000 IRQ during the installation of the new handler. */
    if (en)
    {
        port_DisableEXT_IRQ();
    }
    port_deca_isr = deca_isr;
    if (en)
    {
        port_EnableEXT_IRQ();
    }
}