
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_hal.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "lcd5110.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
LCD5110_display lcd;

void print_lcd(char* msg[]) {
  LCD5110_clear_scr(&lcd);
  LCD5110_set_cursor(10, 5, &lcd);
  LCD5110_print(msg, BLACK, &lcd);    
  LCD5110_refresh(&lcd);
}

typedef enum state_t{
  IDLE_S,
  WAITING_FOR_ECHO_START_S,
  WAITING_FOR_ECHO_STOP_S,
  ECHO_TIMEOUT_S,
  ECHO_NOT_WENT_LOW_S,
  READING_DATA_S,
  ERROR_S
} state_t;

volatile state_t state = IDLE_S;
volatile uint32_t measured_time;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM3) {

    if (state == WAITING_FOR_ECHO_START_S) {
      state = ECHO_TIMEOUT_S;
    }
    
    if (state == WAITING_FOR_ECHO_STOP_S) {
      state = ECHO_NOT_WENT_LOW_S;
    }
  }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
 if (htim->Instance == TIM3) {
  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
   
   __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
   
   if (state == WAITING_FOR_ECHO_START_S) {
    state = WAITING_FOR_ECHO_STOP_S;
   }
  }

  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
   if (state == WAITING_FOR_ECHO_STOP_S) {
    measured_time = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
    state = READING_DATA_S;
   }
  }
 }
}

void on_error (const char* text, bool hang) {

	print_lcd(text);

  if (hang) {
    while (1);
  }

  else {
    state = IDLE_S;
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

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
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  lcd.hw_conf.spi_handle = &hspi2;
  lcd.hw_conf.spi_cs_pin = LCD_CS_Pin;
  lcd.hw_conf.spi_cs_port = LCD_CS_GPIO_Port;
  lcd.hw_conf.rst_pin = LCD_RST_Pin;
  lcd.hw_conf.rst_port = LCD_RST_GPIO_Port;
  lcd.hw_conf.dc_pin = LCD_DC_Pin;
  lcd.hw_conf.dc_port = LCD_DC_GPIO_Port;
  lcd.def_scr = lcd5110_def_scr;
  LCD5110_init(&lcd.hw_conf, LCD5110_INVERTED_MODE, 0x40, 2, 3);

  if (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin)) {
    state = ERROR_S;
    on_error("Error -- Echo line is high", true);
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
  __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);

  while (1)
  {
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
    state = WAITING_FOR_ECHO_START_S;
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    
    while (TIM1->CR1 & TIM_CR1_CEN);

    if (HAL_GPIO_ReadPin (TRIG_GPIO_Port, TRIG_Pin))
    {
      print_lcd("Trigger didnt go low!");
      HAL_NVIC_DisableIRQ(TIM3_IRQn);
      state = IDLE_S;
      HAL_NVIC_EnableIRQ(TIM3_IRQn);
      continue;
    }

    while (state != READING_DATA_S &&
        state != ECHO_TIMEOUT_S &&
        state != ECHO_NOT_WENT_LOW_S
        );
      HAL_NVIC_DisableIRQ(TIM3_IRQn);
      state_t state_copy = state;
      state = IDLE_S;
      HAL_NVIC_EnableIRQ(TIM3_IRQn);

    if (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin))
    {
      print_lcd("Error -- unmeasured distance");
      HAL_Delay(100);
      continue;
    }

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    if (state_copy == ECHO_TIMEOUT_S)
    {
      print_lcd("Echo timeout!\n");
    }
    else
    {
      if (measured_time > TOO_FAR_TIMEOUT)
      {
        print_lcd("Too far -- no echo received");
      } else {
        float distance = (measured_time * 10) / 58;
        char unit[] = "mm";
        if (distance > 1000) {
          distance /= 1000;
          unit[0] = 'm';
          unit[1] = '\0';
        }
        if (distance > 100) {
          distance /= 10;
          unit[0] = 'c';
          unit[1] = 'm';
        }
        LCD5110_clear_scr(&lcd);
        LCD5110_set_cursor(2, 2, &lcd);
        LCD5110_printf(&lcd, BLACK, "Distance:\n%.2f %s", distance, unit);
        LCD5110_set_cursor(2, (LCD5110_get_cursor(&lcd).y + 16), &lcd);
        LCD5110_printf(&lcd, BLACK, "Resp. time:\n %lu mks\n", measured_time);
        LCD5110_refresh(&lcd);
      }
    }
    HAL_Delay(50);

  }

  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
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
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
