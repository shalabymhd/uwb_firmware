/**
  ******************************************************************************
  * @file    bias.h
  * @brief   This file contains all the function prototypes for
  *          the bias.c file
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BIAS_H__
#define __BIAS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "common.h"
#include "deca_regs.h"

/* Typedef -------------------------------------------------------------------*/

/* Function Prototypes -------------------------------------------------------*/

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: timestampReceivedFrame()
 *
 * @brief This function retrieves the estimated first path signal power (FPP) as per
 *        Decawave's documentation. The addressed signal is the most recently received. 
 * 
 * @return (double) FPP.
 */
double retrieveFPP(void);


#ifdef __cplusplus
}
#endif

#endif /* __BIAS_H__ */