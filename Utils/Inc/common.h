/*
 * dwt_stm32_delay.h
 *
 *  Created on: Mar 16, 2018
 *      Author: jcano
 */

#ifndef COMMON_DWT_STM32_DELAY_H_
#define COMMON_DWT_STM32_DELAY_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stm32f4xx_hal.h"

/* Returns the interval between previous_ticks and current ticks. */
uint32_t getInterval(uint32_t* previous_ticks_buff);

#ifdef __cplusplus
}
#endif


#endif /* COMMON_DWT_STM32_DELAY_H_ */
