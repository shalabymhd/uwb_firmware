/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  * @attention
  *
  * This script uses the hash table implementation extracted from
  * Copyright (c) 2005-2021, Troy D. Hanson http://troydhanson.github.io/uthash/
  * All rights reserved.
  * 
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uthash.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

// hash table implementation to store integer parameters
struct int_params {
    char key[10];      /* field used as the key */
    uint8_t value;     /* field used to store integers */
    UT_hash_handle hh; /* makes this structure hashable */
};

// hash table implementation to store float parameters
struct float_params {
    char key[10];      /* field used as the key */
    float value;       /* field used to store integers */
    UT_hash_handle hh; /* makes this structure hashable */
};

// hash table implementation to store boolean parameters
struct bool_params {
    char key[10];      /* field used as the key */
    bool value;     /* field used to store booleans*/
    UT_hash_handle hh; /* makes this structure hashable */
};

// hash table implementation to store string parameters
struct str_params {
    char key[10];      /* field used as the key */
    char value[20];      /* field used to store string messages */
    UT_hash_handle hh; /* makes this structure hashable */
};
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USB_DM_Pin GPIO_PIN_11
#define USB_DM_GPIO_Port GPIOA
#define USB_DP_Pin GPIO_PIN_12
#define USB_DP_GPIO_Port GPIOA

#define BOARD_ID (7) // Module's ID.
#define USB_BUFFER_SIZE (160) // Size of the USB buffer, in bytes.
                              // TODO: to be modified?? 

/* USER CODE BEGIN Private defines */
#define DW_RESET_Pin GPIO_PIN_11
#define DW_RESET_GPIO_Port GPIOC

// To be used in deca_mutex
#if !(EXTI9_5_IRQn)
#define DECAIRQ_EXTI_IRQn       (23)
#else
#define DECAIRQ_EXTI_IRQn       (EXTI9_5_IRQn)
#endif

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
