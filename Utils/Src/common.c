#include <stdio.h>
#include "common.h"

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

void convertFloatToString(char* str,float data){
	int int_part, dec_part;
	int_part = (int) data;
	dec_part = (int) ((data-(float)int_part)*10000.0);

	if (dec_part < 0 && int_part >= 0){
		sprintf(str,(char*)"-%d.%d",int_part,-dec_part);
	}
	else if (dec_part < 0){
		sprintf(str,(char*)"%d.%d",int_part,-dec_part);
	}
	else{
		sprintf(str,(char*)"%d.%d",int_part,dec_part);
	} 
}

void convertElementR3ToString(char* str, element_R3 data){
	char x_str[5], y_str[5], z_str[5];

	convertFloatToString(x_str, data.x);
	convertFloatToString(y_str, data.y);
	convertFloatToString(z_str, data.z);

	sprintf(str,(char*)"[%s, %s, %s]",x_str,y_str,z_str);
}
