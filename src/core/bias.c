/**
  ******************************************************************************
  * @file    bias.c
  * @brief   This file provides code for everything UWB-bias related.
  ******************************************************************************
  */

// TODO: The following two constants are hard-coded, but could be automatically 
// adjusted using the config parameter used to initialize the UWB chips in 
// dwt_general.c.
#define A_CONSTANT (121.74) // 113.77 for PRF of 16 MHz, 121.74 for PRF of 64 MHz
#define N_ADJUSTMENT (-10) // SFD LENGTH-2 and adjustment as per SFD defined sequence

/* Includes ------------------------------------------------------------------*/
#include "bias.h"
#include <math.h>

/* MAIN BIAS FUNCTIONS ---------------------------------------- */ 

int retrieveDiagnostics(float* fpp, float* rxp, uint16_t* noise_std){
    uint8_t F_reg_data[RX_FQUAL_LEN] = {0};
    uint16_t F1, F2, F3, cir_pr;

    /* Read the diagnostics register and save to local memory */
    dwt_readfromdevice(RX_FQUAL_ID, RX_FQUAL_OFFSET, RX_FQUAL_LEN, F_reg_data);

    /* Get the first path amplitudes */
    F1 = dwt_read16bitoffsetreg(RX_TIME_ID, RX_TIME_FP_AMPL1_OFFSET); // Point 1
    F2 = (*(uint64_t*)F_reg_data & FP_AMPL2_MASK) >> FP_AMPL2_SHIFT; // Point 2
    F3 = (*(uint64_t*)F_reg_data & FP_AMPL3_MASK) >> FP_AMPL3_SHIFT; // Point 3

    /* Get the CIR power */
    cir_pr = (*(uint64_t*)F_reg_data & CIR_MXG_MASK) >> CIR_MXG_SHIFT;

    /* Get the noise STD */
    *noise_std = (*(uint64_t*)F_reg_data & STD_NOISE_MASK) >> STD_NOISE_SHIFT;

    /* Get the Preamble Accumulation Count */
    uint8_t N_reg_data[RX_FINFO_LEN];
    uint16_t N;

    dwt_readfromdevice(RX_FINFO_ID, RX_FINFO_OFFSET, RX_FINFO_LEN, N_reg_data); // Read the entire register
    N = (*(uint32_t*)N_reg_data & RX_FINFO_RXPACC_MASK) >> RX_FINFO_RXPACC_SHIFT; // Retrieve the subregister for N
    N = N + N_ADJUSTMENT; // This is the adjustment for the SFD accumulation as per the manual.
                          // TODO: compare to RXPACC_NOSAT before implementing?

    /* Compute the first path power */
    *fpp = 10*log10((F1*F1 + F2*F2 + F3*F3)/(N*N)) - A_CONSTANT;

    /* Compute the receive signal power */ 
    *rxp = 10*log10((cir_pr * pow(2,17))/(N*N)) - A_CONSTANT;

    return 1;
}
