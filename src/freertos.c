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
#include "usb_interface.h"
#include "commands.h"

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
char CdcReceiveBuffer[USB_BUFFER_SIZE]; // buffer to store received USB data.
struct int_params *FSM_int_params = NULL;
struct float_params *FSM_float_params = NULL;
struct bool_params *FSM_bool_params = NULL;
struct str_params *FSM_str_params = NULL;
/* USER CODE END Variables */

osThreadId defaultTaskHandle;
osThreadId blinkTaskHandle;
osThreadId usbReceiveTaskHandle;
osThreadId twrInterruptTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartDefaultTask(void const * argument);
void StartBlinking(void const * argument);
void StartUsbReceive(void const * argument);
void uwbInterruptTask(void const * argument);
/* USER CODE END FunctionPrototypes */

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

  osThreadDef(usbReceive, StartUsbReceive, osPriorityAboveNormal, 0, 1024);
  usbReceiveTaskHandle = osThreadCreate(osThread(usbReceive), NULL);

  osThreadDef(twrInterrupt, uwbInterruptTask, osPriorityRealtime, 0, 256);
  twrInterruptTaskHandle = osThreadCreate(osThread(twrInterrupt), NULL);
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
} // end StartDefaultTask()

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartBlinking(void const *argument){
  while (1){
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
    osDelay(250);
  }
} // end StartBlinking()

void StartUsbReceive(void const *argument){
  // To receive the data transmitted by a computer, execute in a terminal
  // >> cat /dev/ttyACMx

  uint8_t reg_state; // to store the state of the DW receiver
  interfaceInit();
  
  while (1){
    /* Read the USB buffer */
    readUsb();

    /* RX is supposed to be enabled from the interrupt task. If not, re-enable */
    reg_state = dwt_read8bitoffsetreg(SYS_STATE_ID, 1); // read RX status
    if (!reg_state){
      dwt_rxenable(DWT_START_RX_IMMEDIATE); // turn on uwb receiver
    } 

    osDelay(1); 
  }
} // end StartUsbReceive()

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void uwbInterruptTask(void const *argument){

  dwt_setrxaftertxdelay(40);

  while (1){ 
    uwbFrameHandler(); 
    dwt_rxenable(DWT_START_RX_IMMEDIATE); // turn on uwb receiver
  }
} // end uwbInterruptTask()
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
