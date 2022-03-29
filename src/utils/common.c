/**
  ******************************************************************************
  * @file    common.c
  * @brief   This file provides general code to be used by all submodules.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <math.h>   
#include "common.h"

void convert_float_to_string(char* str,float data){
  char *tmpSign = (data < 0) ? "-" : "";
  float tmpVal = (data < 0) ? -data : data;

  int tmpInt1 = tmpVal;                  // Get the integer (678).
  float tmpFrac = tmpVal - tmpInt1;      // Get fraction (0.0123).
  int tmpInt2 = trunc(tmpFrac * 10000);  // Turn into integer (123).

  // Print as parts, note that you need 0-padding for fractional bit.
  sprintf(str, "%s%d.%04d", tmpSign, tmpInt1, tmpInt2);
}

void convert_elementR3_to_string(char* str, element_R3 data){
	char x_str[5], y_str[5], z_str[5];

	convert_float_to_string(x_str, data.x);
	convert_float_to_string(y_str, data.y);
	convert_float_to_string(z_str, data.z);

	sprintf(str,(char*)"[%s, %s, %s]",x_str,y_str,z_str);
}

void usb_print(char* c){
  // TODO: can this be overloaded so that we can allow a format specifier like
  // sprintf? 
  CDC_Transmit_FS((unsigned char*) c, strlen(c));
}

/**
  * @brief  Checks whether the specified EXTI line is enabled or not.
  * @param  EXTI_Line: specifies the EXTI line to check.
  *   This parameter can be:
  *     @arg EXTI_Linex: External interrupt line x where x(0..19)
  * @retval The "enable" state of EXTI_Line (SET or RESET).
  */
ITStatus EXTI_GetITEnStatus(uint32_t x)
{
    return ((NVIC->ISER[(((uint32_t)x) >> 5UL)] &\
            (uint32_t)(1UL << (((uint32_t)x) & 0x1FUL)) ) == (uint32_t)RESET)?(RESET):(SET);
}