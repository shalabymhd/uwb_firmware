#include "stm32f4xx_hal.h"

uint32_t DWT_Delay_Init(void);

/* Returns the interval between previous_ticks and current ticks. */
uint32_t getInterval(uint32_t* previous_ticks_buff);