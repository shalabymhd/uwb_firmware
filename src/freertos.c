/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "common.h"
#include "MPU9250.h"
#include "ranging.h"
#include "spi.h"
#include "testing.h"
#include "stm32f4xx_it.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t CdcReceiveBuffer[224]; 
/* USER CODE END Variables */

osThreadId defaultTaskHandle;
osThreadId blinkTaskHandle;
osThreadId usbTransmitTaskHandle;
osThreadId usbReceiveTaskHandle;
osThreadId imuTaskHandle;
osThreadId uwbTaskHandle;
osThreadId listeningTaskHandle;
osThreadId uwbTestingTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartBlinking(void const * argument);
void StartUsbTransmit(void const * argument);
void StartUsbReceive(void const * argument);
void StartImuTask(void const * argument);
void StartUwbTask(void const * argument);
void StartListeningTask(void const * argument);
void StartUwbTesting(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  // osThreadDef(defaultTask, StartDefaultTask, osPriorityIdle, 0, 128);
  // defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(blink, StartBlinking, osPriorityIdle, 0, 128);
  blinkTaskHandle = osThreadCreate(osThread(blink), NULL);

  osThreadDef(usbReceive, StartUsbReceive, osPriorityNormal, 0, 128);
  usbReceiveTaskHandle = osThreadCreate(osThread(usbReceive), NULL);
  /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartBlinking(void const *argument){
  while (1){
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
    osDelay(1000);
  }
}

void StartUsbReceive(void const *argument){
  // To receive the data transmitted by a computer, execute in a terminal
  // >> cat /dev/ttyACMx
  while (1){
    
    char *idx_beg;
    char *idx_end;

    char print_stat[20]; 

    idx_beg = &CdcReceiveBuffer;
    idx_end = strstr(CdcReceiveBuffer, "\n");
  
    uint8_t len = idx_end - idx_beg - 1; // Removing the first entry 
    if (idx_end > 0)
    {
      uint8_t *res = (uint8_t*)malloc(sizeof(uint8_t)*(len+1));

      /* if the memory has not been allocated, interrupt operations */
      if (res == NULL) {MemManage_Handler();}

      strncpy(res, idx_beg + 1, len);
      res[len] = '\0';
      sprintf(print_stat, "'%s'\n", res);
      usb_print(print_stat);

      free(res);

      /* Update the buffer */
      res = (uint8_t*)malloc(sizeof(uint8_t)*(224 - 1));
      if (res == NULL) {MemManage_Handler();}
      memcpy(res, CdcReceiveBuffer + len + 2, 224 - len - 2);
      memset(CdcReceiveBuffer + 1, '\0', 224 - 1); // clear the buffer
      memcpy(CdcReceiveBuffer + 1, res, 224 - len - 1);
      CdcReceiveBuffer[0] = CdcReceiveBuffer[0] - len - 1;

      free(res);
    }

    // memset(CdcReceiveBuffer, '\0', 224); // clear the buffer
    osDelay(1000);
  }
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
