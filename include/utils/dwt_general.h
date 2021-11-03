#include "stm32f4xx_hal.h"

typedef int decaIrqStatus_t ; // Type for remembering IRQ status

/* DW1000 IRQ (EXTI9_5_IRQ) handler type. */
typedef void (*port_deca_isr_t)(void);

uint32_t DWT_Delay_Init(void);

/* Returns the interval between previous_ticks and current ticks. */
uint32_t getInterval(uint32_t* previous_ticks_buff);

void port_DisableEXT_IRQ(void);
void port_EnableEXT_IRQ(void);
uint32_t port_GetEXT_IRQStatus(void);

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn decamutexon()
 *
 * @brief This function should disable interrupts. This is called at the start of a critical section
 * It returns the IRQ state before disable, this value is used to re-enable in decamutexoff call
 *
 * Note: The body of this function is defined in deca_mutex.c and is platform specific
 *
 * input parameters:
 *
 * output parameters
 *
 * returns the state of the DW1000 interrupt
 */
decaIrqStatus_t decamutexon(void) ;

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn decamutexoff()
 *
 * @brief This function should re-enable interrupts, or at least restore their state as returned(&saved) by decamutexon
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
void decamutexoff(decaIrqStatus_t s) ;