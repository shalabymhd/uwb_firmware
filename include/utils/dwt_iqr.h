/**
  ******************************************************************************
  * @file    dwt_iqr.h
  * @brief   This file contains all the function prototypes for
  *          the dwt_iqr.c file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DWT_IQR_H__
#define __DWT_IQR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Type defines --------------------------------------------------------------*/
typedef int decaIrqStatus_t ; // Type for remembering IRQ status
typedef void (*port_deca_isr_t)(void); // DW1000 IRQ (EXTI9_5_IRQ) handler type.

/* Defines -------------------------------------------------------------------*/

/* Variable Declarations -----------------------------------------------------*/
// port_deca_isr_t port_deca_isr; // DW1000 IRQ handler declaration.

/* Function Prototypes -------------------------------------------------------*/
void port_DisableEXT_IRQ(void);
void port_EnableEXT_IRQ(void);
uint32_t port_GetEXT_IRQStatus(void);

uint32_t port_CheckEXT_IRQ(void);
void process_deca_irq(void);

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_deca_isr()
 *
 * @brief This function is used to install the handling function for DW1000 IRQ.
 *
 * NOTE:
 *   - As EXTI9_5_IRQHandler does not check that port_deca_isr is not null, the user application must ensure that a
 *     proper handler is set by calling this function before any DW1000 IRQ occurs!
 *   - This function makes sure the DW1000 IRQ line is deactivated while the handler is installed.
 *
 * @param deca_isr function pointer to DW1000 interrupt handler to install
 *
 * @return none
 */
void port_set_deca_isr(port_deca_isr_t deca_isr);

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

#ifdef __cplusplus
}
#endif

#endif /* __DWT_IQR_H__ */