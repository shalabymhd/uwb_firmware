#include "common.h"

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
