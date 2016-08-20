/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
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
#include "stm32f0xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

uint8_t writedata=0;

#define WREN  6
#define WRDI  4
#define RDSR  5
#define WRSR  1
#define READ  3
#define WRITE 2

typedef struct
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t date;
	uint8_t months;
	uint8_t year;
	uint8_t ctrl_reg;
}tRTCReg;

typedef struct
{
	uint8_t address;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t date;
	uint8_t months;
	uint8_t year;
	uint8_t ctrl_reg;
}tRTCRegWrite;

char HexToAscii (char ch)
{
	char c;
    c = ch;
	if (c > 9)
        c = c - 10 + 'A';
	else
	    c = c + '0';
    return c;
}

char GetBCDHighToAscii (char ch)
{
	char c;
    c = (ch >> 4) & 0x0f;

    return HexToAscii (c);
}

char GetBCDLowToAscii (char ch)
{
    return HexToAscii (ch & 0x0f);
}


void bcdtos(char* s, unsigned char bcdchar)
{
	s[0] = GetBCDHighToAscii (bcdchar);
	s[1] = GetBCDLowToAscii (bcdchar);
}

uint8_t read_eeprom(int EEPROM_address)
{
	uint8_t printdata[2];
  //READ EEPROM
	uint8_t data[10];
  uint8_t readsequence[3]={READ, 0, 0};
	readsequence[1]=EEPROM_address>>8;
	readsequence[2]=EEPROM_address;
	
	HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, readsequence, 3, HAL_MAX_DELAY); 
	HAL_SPI_Receive(&hspi1, data, 10, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_SET);
	
	//print data
	bcdtos(printdata,data[3]);
	HAL_UART_Transmit(&huart2, "\r\n", 2, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, "ReadData : ", 11, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, printdata, 2, HAL_MAX_DELAY);
	
  return data[3];
}

void write_eeprom(int EEPROM_address, uint8_t data)
{
	uint8_t printdata[2];
  //READ EEPROM
	uint8_t wrendata=WREN;
  uint8_t writesequence[3]={WRITE, 0, 0};
	writesequence[1]=EEPROM_address>>8;
	writesequence[2]=EEPROM_address;
	
	HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, &wrendata, 1, HAL_MAX_DELAY); 
	HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_SET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, writesequence, 3, HAL_MAX_DELAY); 
  HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_SET);
	
	//print data 
	bcdtos(printdata,data);
	HAL_UART_Transmit(&huart2, "\r\n", 2, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, "WriteData : ", 11, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, printdata, 2, HAL_MAX_DELAY);
	HAL_Delay(100);
}



void printdatetime(tRTCReg* pthisreg)
{
	uint8_t time[]="HH:MM:SS";
	uint8_t date[]="DD/MM/20YY";
	uint8_t day;
	uint8_t isclockhalt;
	uint8_t is12hoursmode;
	bcdtos(&time[0], pthisreg->hours);
	bcdtos(&time[3], pthisreg->minutes);
	bcdtos(&time[6], pthisreg->seconds);
	
	bcdtos(&date[0], pthisreg->date);
	bcdtos(&date[3], pthisreg->months);
	bcdtos(&date[8], pthisreg->year);

	HAL_UART_Transmit(&huart2, "\r\n", 2, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, "CurrentDate : ", 14, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, date, 10, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, "\r\n", 2, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, "CurrentTime : ", 14, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, time, 8, HAL_MAX_DELAY);
}

void SetDateTime()
{
	tRTCRegWrite RTCReg;
	
	RTCReg.address  = 0x00;
	RTCReg.seconds 	= 0x00;
	RTCReg.minutes	= 0x00;
	RTCReg.hours	= 0x12;
	RTCReg.day 	= 0x01;
	RTCReg.date 	= 0x11;
	RTCReg.months 	= 0x07;
	RTCReg.year 	= 0x16;
	HAL_I2C_Master_Transmit(&hi2c1, 0xd0, (uint8_t*)&RTCReg, 8, HAL_MAX_DELAY);
}


/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
	tRTCReg datetiemreg;
	uint8_t data[8]={0x00, 0x01, 0x02,0x03,0x04,0x05,0x06,0xff};
	uint8_t spidata[]={0x05,0xff,0xff};
	uint8_t thisdata;
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
	write_eeprom(1, 0xab);
	thisdata=read_eeprom(1);
  /* USER CODE BEGIN 2 */
	//HAL_I2C_Master_Transmit(&hi2c1, 0xd0, data, 8, HAL_MAX_DELAY);
	//HAL_SPI_TransmitReceive(&hspi1, spidata, spidata, 1, HAL_MAX_DELAY);
	//HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_RESET);
	//HAL_SPI_Transmit(&hspi1, spidata, 2, HAL_MAX_DELAY); 
	//HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_SET);
	
	//HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_RESET);
	//HAL_SPI_Transmit(&hspi1, spidata, 2, HAL_MAX_DELAY); 
	//HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_SET);

	//printf("HELLO WORLD");
	HAL_UART_Transmit(&huart2, "HELLO WORLD", 11, HAL_MAX_DELAY);
	SetDateTime();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	HAL_Delay(2000);
	HAL_I2C_Master_Transmit(&hi2c1, 0xd0, data, 1, HAL_MAX_DELAY);
	HAL_I2C_Master_Receive(&hi2c1, 0xd0, (uint8_t*)&datetiemreg, 7, HAL_MAX_DELAY);
	printdatetime(&datetiemreg);
	writedata++;
	write_eeprom(1, writedata);
	read_eeprom(1);
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* I2C1 init function */
static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure Analogue filter 
    */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CSPIN_Pin */
  GPIO_InitStruct.Pin = CSPIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(CSPIN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CSPIN_GPIO_Port, CSPIN_Pin, GPIO_PIN_RESET);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
