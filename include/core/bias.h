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
 * Function: retrievePower()
 *
 * @brief This function retrieves the following metric as per Decawave's documentation:
 *          1) first path power (fpp), as per Section 4.7.1,
 *        The addressed signal is the most recently received. 
 *  
 * @param fpp (float*) A pointer to where the fpp will be stored.
 * 
 * NOTE: This function and the corresponding notation is primarily based on Section 4.7
 *       in the DW1000 User Manual.
 * 
 * @return (int) 1.
 */
int retrievePower(float*);
int retrieveSkew(float*);


#ifdef __cplusplus
}
#endif

#endif /* __BIAS_H__ */