/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "can.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* CAN���������ú��� */
static void CANFilter_Config(void)
{
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = 0;                          // CAN��������ţ����?0-27
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;      // CAN������ģʽ������ģʽ���б�ģʽ
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;     // CAN�������߶ȣ�16λ��32λ
    sFilterConfig.FilterIdHigh = 0x0000;              // 32λ�£��洢Ҫ���˵�ID�ĸ�16λ
    sFilterConfig.FilterIdLow = 0x0000;                    // 32λ�£��洢Ҫ���˵�ID�ĵ�16λ
    sFilterConfig.FilterMaskIdHigh = 0x0000;               // ����ģʽ�£��洢��������?
    sFilterConfig.FilterMaskIdLow = 0x0000;                // ����ģʽ�£��洢��������?
    sFilterConfig.FilterFIFOAssignment = 0;                // ����ͨ��������ƥ��󣬴洢���ĸ�FIFO
    sFilterConfig.FilterActivation = ENABLE;               // ���������??
    sFilterConfig.SlaveStartFilterBank = 0;

    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        //printf("CAN Filter Config Fail!\r\n");
        Error_Handler();
    }

    //printf("CAN Filter Config Success!\r\n");
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    uint16_t len = 0;
		CAN_RxHeaderTypeDef can_Rx;
		uint8_t recvBuf[8];
    
    HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &can_Rx, recvBuf);

//    if(can_Rx.IDE == CAN_ID_STD)
//    {
//        len += sprintf((char *)&uartBuf[len], "��׼ID��%#X; ", can_Rx.StdId);
//    }
//    else if(can_Rx.IDE == CAN_ID_EXT)
//    {
//        len += sprintf((char *)&uartBuf[len], "��չID��%#X; ", can_Rx.ExtId);
//    }
//    
//    if(can_Rx.RTR == CAN_RTR_DATA)
//    {
//        len += sprintf((char *)&uartBuf[len], "����֡; ����Ϊ��");
//        
//        for(int i = 0; i < can_Rx.DLC; i ++)
//        {
//            len += sprintf((char *)&uartBuf[len], "%X ", recvBuf[i]);
//        }
//        
//        len += sprintf((char *)&uartBuf[len], "\r\n");
//        HAL_UART_Transmit(&huart1, uartBuf, len, 100);        
//    }
//    else if(can_Rx.RTR == CAN_RTR_REMOTE)
//    {
//        len += sprintf((char *)&uartBuf[len], "ң��֡\r\n");
//        HAL_UART_Transmit(&huart1, uartBuf, len, 100);        
//    }    
}



void cansend(uint32_t id, uint8_t* data, uint8_t dlc) {
  CAN_TxHeaderTypeDef txHeader;
  uint32_t txMailbox;

  // 设置CAN报文�??
  txHeader.StdId = id;          // 设置标准标识�??
  txHeader.ExtId = 0x00;           // 设置扩展标识�??
  txHeader.RTR = CAN_RTR_DATA;  // 数据�??
  txHeader.IDE = CAN_ID_STD;    // 标准标识�??
  txHeader.DLC = dlc;           // 数据长度

  // 发�?�CAN数据
  if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, data, &txMailbox) != HAL_OK) {
      while (1);
      // 处理发�?�错�??
  }
}

void send_hex_data() {
  uint8_t data[8];
  data[0] = 0x12;  // 数据的高字节
  data[1] = 0x34;  // 数据的低字节

  cansend(0x125, data, 8);  // 发�?�数据，ID�??0x123，数据为�?? 0x 数据长度�??2
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
  MX_CAN1_Init();
  /* USER CODE BEGIN 2 */

  HAL_GPIO_WritePin(HCAN_RS_EN1_GPIO_Port, HCAN_RS_EN1_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(CHAN_EN_GPIO_Port, CHAN_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CHAN_A0_GPIO_Port, CHAN_A0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(CHAN_A1_GPIO_Port, CHAN_A1_Pin, GPIO_PIN_RESET);

  // OBD 6 -> 7
  HAL_GPIO_WritePin(PORT0_EN_GPIO_Port, PORT0_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT0_A0_GPIO_Port, PORT0_A0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PORT0_A1_GPIO_Port, PORT0_A1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT0_A2_GPIO_Port, PORT0_A2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT0_A3_GPIO_Port, PORT0_A3_Pin, GPIO_PIN_RESET);

  // OBD 14 -> 15
  HAL_GPIO_WritePin(PORT1_EN_GPIO_Port, PORT1_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT1_A0_GPIO_Port, PORT1_A0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PORT1_A1_GPIO_Port, PORT1_A1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT1_A2_GPIO_Port, PORT1_A2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT1_A3_GPIO_Port, PORT1_A3_Pin, GPIO_PIN_SET);
	

	CANFilter_Config();
	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

	
	if(HAL_CAN_Start(&hcan1) != HAL_OK){
    while (1); 
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    HAL_GPIO_WritePin(LED_CTR_GPIO_Port, LED_CTR_Pin, GPIO_PIN_SET);
    HAL_Delay(500);   
    HAL_GPIO_WritePin(LED_CTR_GPIO_Port, LED_CTR_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
		

    send_hex_data();

		
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 240;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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
