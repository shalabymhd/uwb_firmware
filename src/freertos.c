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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define INACTIVE_STATE (0)
#define GET_ID_STATE (1)
#define RANGING_STATE (2)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
char CdcReceiveBuffer[USB_BUFFER_SIZE]; // buffer to store received USB data.
uint8_t FSM_status; // pointer to the status of the finite state machine.
                    // 0 = inactive, tag in receive mode
                    // 1 = initiate an instance two-way ranging
                    // 2 = initiate two-way ranging indefinitely

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
void twrInterruptTask(void const * argument);
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

  osThreadDef(usbReceive, StartUsbReceive, osPriorityAboveNormal, 0, 512);
  usbReceiveTaskHandle = osThreadCreate(osThread(usbReceive), NULL);

  osThreadDef(twrInterrupt, twrInterruptTask, osPriorityRealtime, 0, 256);
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
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
  }
} // end StartBlinking()

void StartUsbReceive(void const *argument){
  // To receive the data transmitted by a computer, execute in a terminal
  // >> cat /dev/ttyACMx

  bool success;
  decaIrqStatus_t stat;

  // FSM_status = 0; // setting the initial state of the FSM to be inactive
  while (1){
    stat = decamutexon();
    readUsb();
    decamutexoff(stat);

    switch (FSM_status)
    {
      case 0:
        /* code */
        break;
      
      case 1:
        /* code */
        break;
      
      case 2:
        // usb_print("Status set to RANGING!\r\n"); // placeholder
        success = twrInitiateInstance();

        if (success){ 
          usb_print("TWR SUCCESS!\r\n"); // placeholder
          FSM_status = 0;
        }
        else {
          usb_print("TWR FAIL!\r\n");
        }
        break;

      default:
        break;
    }
    osDelay(1); // TODO: to be modified?? 
  }
} // end StartUsbReceive()

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void twrInterruptTask(void const *argument){
  decaIrqStatus_t stat;

  dwt_setrxaftertxdelay(40);

  while (1){
    osThreadSuspend(NULL); // suspend the thread, re-enabled using uwb receive interrupt
    
    /* Executes right after a dw1000 receive interrupt */
    stat = decamutexon(); // disable dw1000 interrupts
    twrReceiveCallback(); // complete TWR 
    decamutexoff(stat); // re-enable dw1000 interrupts
    dwt_rxenable(DWT_START_RX_IMMEDIATE); // turn on uwb receiver
  }
} // end twrInterruptTask()
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
