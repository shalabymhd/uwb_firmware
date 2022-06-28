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
#include "deca_device_api.h"

/* Typedef -------------------------------------------------------------------*/

/* Function Prototypes -------------------------------------------------------*/

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: retrieveDiagnostics()
 *
 * @brief This function retrieves the estimated first path signal power (FPP) as per
 *        Decawave's documentation. The addressed signal is the most recently received. 
 *  
 * NOTE: This function and the corresponding notation is primarily based on Section 4.7.1
 *       in the DW1000 User Manual.
 * 
 * @return (float) FPP.
 */
float retrieveDiagnostics(void);


#ifdef __cplusplus
}
#endif

#endif /* __BIAS_H__ */