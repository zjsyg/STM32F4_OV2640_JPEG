/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "ov2640.h"
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

/* USER CODE BEGIN PV */
#define JPEG_W  320
#define JPEG_H  240

// uint8_t  jpeg_data[10*1024];
// uint16_t  jpeg_buf[JPEG_W*JPEG_H/2];
union 
{
  uint16_t jpeg_buf[JPEG_W*JPEG_H/2];
  uint8_t jpeg_data[10*1024];
}jpeg_data_buf;
uint32_t  jpeg_size = JPEG_W*JPEG_H/2;
uint8_t jpeg_ok = 0;
uint32_t jpeg_len = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay_us(uint16_t us)
{
	uint16_t differ=0xffff-us-5; 
	HAL_TIM_Base_Start(&htim6);
	__HAL_TIM_SetCounter(&htim6,differ); 
	while(differ < 0xffff-5) 
	{ 
		differ = __HAL_TIM_GetCounter(&htim6); 
	} 
	HAL_TIM_Base_Stop(&htim6);
 
}

int fputc(int ch,FILE *f)
{
  HAL_UART_Transmit(&huart1,(uint8_t *)(&ch),sizeof(ch),HAL_MAX_DELAY);
  return ch;
}

uint8_t frame = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim7)
    {
        // printf("frame:%dfps\r\n", frame);
        frame = 0;
    }
}

void jpeg_data_process(void)
{
	if(jpeg_ok == 0)
	{
    HAL_DMA_Abort(hdcmi.DMA_Handle);
		jpeg_len=jpeg_size - __HAL_DMA_GET_COUNTER(hdcmi.DMA_Handle);
		jpeg_ok=1;
	}
	if(jpeg_ok==2)
	{
    HAL_DMA_Start(hdcmi.DMA_Handle,(uint32_t)&hdcmi.Instance->DR,(uint32_t)&jpeg_data_buf.jpeg_buf,jpeg_size);
		jpeg_ok=0;
	}
} 

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
  frame++;
  HAL_GPIO_TogglePin(GPIOF,GPIO_PIN_9);
  __HAL_DCMI_CLEAR_FLAG(hdcmi,DCMI_FLAG_FRAMERI);
  jpeg_data_process();
  __HAL_DCMI_ENABLE_IT(hdcmi,DCMI_IT_FRAME);
}


void jpeg_display(void)
{
  uint8_t headok = 0;
  uint32_t jpegstart,jpeglen;
  HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_CONTINUOUS,(uint32_t)&jpeg_data_buf.jpeg_buf,jpeg_size);
  if(jpeg_ok==1)
	{
    // for (int i = 0; i < jpeg_len*2; i++)
    // {
    //   jpeg_data[2*i] = jpeg_buf[i];
    //   jpeg_data[2*i+1] = jpeg_buf[i]>>8;
    // }      
    for (int i = 0;i < jpeg_len*2;i++)
    {
      if ((jpeg_data_buf.jpeg_data[i] == 0XFF) && (jpeg_data_buf.jpeg_data[i + 1] == 0XD8))
      {
        jpegstart = i;
        headok = 1;
      }
      if ((jpeg_data_buf.jpeg_data[i] == 0XFF) && (jpeg_data_buf.jpeg_data[i + 1] == 0XD9) && headok)
      {
        jpeglen = i - jpegstart + 2;
        break;
      }
    }
    if (jpeglen)
    {
      for (int i = 0; i < jpeglen; i++)
      {
        USART1->DR = jpeg_data_buf.jpeg_data[i];
        while ((USART1->SR & 0X40) == 0);
      }
    }
    jpeg_ok=2;
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */
  __HAL_DCMI_ENABLE_IT(&hdcmi,DCMI_IT_FRAME);
  HAL_TIM_Base_Start_IT(&htim7);
  while(ov2640_init());
  HAL_Delay(100);
  ov2640_jpeg_mode();
  ov2640_outsize_set(JPEG_W,JPEG_H);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    jpeg_display();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
