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
 * @brief This function retrieves the following three signals as per Decawave's documentation:
 *          1) first path power (fpp), as per Section 4.7.1,
 *          2) receive signal power (rxp), as per Section 4.7.2, 
 *          3) and the standard deviation of the noise (std), as per Register file 0x12.
 *        The addressed signal is the most recently received. 
 *  
 * @param fpp (float*) A pointer to where the fpp will be stored.
 * @param rxp (float*) A pointer to where the rxp will be stored. 
 * @param noise_std (uint16_t*) A pointer to where the noise std will be stored.
 * 
 * NOTE: This function and the corresponding notation is primarily based on Section 4.7
 *       in the DW1000 User Manual.
 * 
 * @return (int) 1.
 */
int retrieveDiagnostics(float*, float*, uint16_t*);
int retrieveSkew(float*);


#ifdef __cplusplus
}
#endif

#endif /* __BIAS_H__ */