/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
# include "myFreeRTOS.h"
# include "adc.h"
# include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
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

volatile uint8_t flagHalf = 0;
volatile uint8_t flagFull = 0;
volatile uint8_t dmaBusyUART = 0;

uint16_t buf_ADC[ADC_BUF_SIZE];



void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	flagHalf = 1;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	flagFull = 1;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) 
	{
		dmaBusyUART = 0;
  }
}

/* USER CODE END Variables */
/* Definitions for ADC_Task */
osThreadId_t ADC_TaskHandle;
uint32_t ADC_TaskBuffer[ 128 ];
osStaticThreadDef_t ADC_TaskControlBlock;
const osThreadAttr_t ADC_Task_attributes = {
  .name = "ADC_Task",
  .cb_mem = &ADC_TaskControlBlock,
  .cb_size = sizeof(ADC_TaskControlBlock),
  .stack_mem = &ADC_TaskBuffer[0],
  .stack_size = sizeof(ADC_TaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TX_UART_Task */
osThreadId_t TX_UART_TaskHandle;
uint32_t TX_UART_TaskBuffer[ 128 ];
osStaticThreadDef_t TX_UART_TaskControlBlock;
const osThreadAttr_t TX_UART_Task_attributes = {
  .name = "TX_UART_Task",
  .cb_mem = &TX_UART_TaskControlBlock,
  .cb_size = sizeof(TX_UART_TaskControlBlock),
  .stack_mem = &TX_UART_TaskBuffer[0],
  .stack_size = sizeof(TX_UART_TaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TX_Queue */
osMessageQueueId_t TX_QueueHandle;
uint8_t TX_QueueBuffer[ 16 * sizeof( ADC_Info_t ) ];
osStaticMessageQDef_t TX_QueueControlBlock;
const osMessageQueueAttr_t TX_Queue_attributes = {
  .name = "TX_Queue",
  .cb_mem = &TX_QueueControlBlock,
  .cb_size = sizeof(TX_QueueControlBlock),
  .mq_mem = &TX_QueueBuffer,
  .mq_size = sizeof(TX_QueueBuffer)
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Start_ADC_Task(void *argument);
void Start_TX_UART_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

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

  /* Create the queue(s) */
  /* creation of TX_Queue */
  TX_QueueHandle = osMessageQueueNew (16, sizeof(ADC_Info_t), &TX_Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of ADC_Task */
  ADC_TaskHandle = osThreadNew(Start_ADC_Task, NULL, &ADC_Task_attributes);

  /* creation of TX_UART_Task */
  TX_UART_TaskHandle = osThreadNew(Start_TX_UART_Task, NULL, &TX_UART_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Start_ADC_Task */
/**
  * @brief  Function implementing the ADC_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Start_ADC_Task */
void Start_ADC_Task(void *argument)
{
  /* USER CODE BEGIN Start_ADC_Task */
	
	HAL_ADC_Start_DMA(&hadc1, (void*)buf_ADC, ADC_BUF_SIZE);
	
  /* Infinite loop */
  for(;;)
  {
		
		if(flagHalf == 1)
	  {
			flagHalf = 0;
			
			uint32_t tempSum = 0;
			
			for(uint8_t i = 0; i < 50; i++)
			{
			  tempSum += buf_ADC[i];	
			}
			
			ADC_Info_t msg;
			msg.avg = (uint16_t)(tempSum / 50);
			msg.half = 1;
			
			osStatus_t status = osMessageQueuePut(TX_QueueHandle, &msg, 0, 0);
			
			if (status == osErrorResource) 
		  {
				osMessageQueueGet(TX_QueueHandle, NULL, NULL, 0);
        osMessageQueuePut(TX_QueueHandle, &msg, 0, 0);
			}
		}
	
		
		if(flagFull == 1)
	  {
			flagFull = 0;
			
			uint32_t tempSum = 0;
			
			for(uint8_t i = 50; i < 100; i++)
			{
			  tempSum += buf_ADC[i];	
			}
			
			ADC_Info_t msg;
			msg.avg = (uint16_t)(tempSum / 50);
			msg.half = 0;
			
			osStatus_t status = osMessageQueuePut(TX_QueueHandle, &msg, 0, 0);
			
			if (status == osErrorResource) 
		  {
				osMessageQueueGet(TX_QueueHandle, NULL, NULL, 0);
        osMessageQueuePut(TX_QueueHandle, &msg, 0, 0);
			}
		}
		
     osDelay(1);
  }
  /* USER CODE END Start_ADC_Task */
}

/* USER CODE BEGIN Header_Start_TX_UART_Task */
/**
* @brief Function implementing the TX_UART_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_TX_UART_Task */
void Start_TX_UART_Task(void *argument)
{
  /* USER CODE BEGIN Start_TX_UART_Task */
	
	ADC_Info_t msg;
  
	/* Infinite loop */
  for(;;)
  {
		if (dmaBusyUART)
		{
			osThreadYield();
      continue;
    }
		
		if (osMessageQueueGet(TX_QueueHandle, &msg, NULL, osWaitForever) == osOK)
		{
      dmaBusyUART = 1;
			static uint8_t tx[sizeof(uint16_t)];
			
			tx[0] = (uint8_t) (msg.avg >> 0x08);
      tx[1] = (uint8_t) (msg.avg);
			
			HAL_UART_Transmit_DMA(&huart1, tx, sizeof(uint16_t));
    }
		
    osThreadYield();
  }
  /* USER CODE END Start_TX_UART_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

