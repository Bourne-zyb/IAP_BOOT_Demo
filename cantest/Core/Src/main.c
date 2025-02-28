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
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "menu.h"
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
extern pFunction JumpToApplication;
extern uint32_t JumpAddress;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void handle_cantest_welcome(void) {
  uint8_t usbBuf[200]; // 用于存储要发送的字符串
  uint16_t len = 0;

  // 欢迎语
  len += sprintf((char *)&usbBuf[len], "Welcome to CAN Communication!\r\n");
  len += sprintf((char *)&usbBuf[len], "Available Commands:\r\n");
  len += sprintf((char *)&usbBuf[len], "1. restart - Reset the CAN receive counter and expected data.\r\n");
  len += sprintf((char *)&usbBuf[len], "2. show    - Display the current CAN receive counter value.\r\n");

  // 使用 CDC_Transmit_FS 发送数据
  CDC_Transmit_FS(usbBuf, len);
}


/* CAN过滤配置函数 */
static void CANFilter_Config(void)
{
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = 0;                          // CAN过滤器编号，范围0-27
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;      // CAN过滤模式，掩码模式或列表模式
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;     // CAN过滤器尺度，16位或32位
		//设置1：接收所有帧
    sFilterConfig.FilterIdHigh = 0x0000;                   // 32位模式，存储要配置的ID的高16位
    sFilterConfig.FilterIdLow = 0x0000;                    // 32位模式，存储要配置的ID的低16位
    sFilterConfig.FilterMaskIdHigh = 0x0000;               // 掩码模式下，存储过滤器掩码高16位
    sFilterConfig.FilterMaskIdLow = 0x0000;                // 掩码模式下，存储过滤器掩码低16位

// 设置2：只接收stdID为奇数的帧
//    sFilterConfig.FilterIdHigh = 0x0020;		             //CAN_FxR1 的高16位
//    sFilterConfig.FilterIdLow = 0x0000;			             //CAN_FxR1 的低16位
//    sFilterConfig.FilterMaskIdHigh = 0x0020;	           //CAN_FxR2的高16位
//    sFilterConfig.FilterMaskIdLow = 0x0000;		           //CAN_FxR2的低16位
	
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;                // 过滤器通道，匹配后存储的FIFO编号
    sFilterConfig.FilterActivation = ENABLE;               // 启用过滤器
    sFilterConfig.SlaveStartFilterBank = 0;								 //从CAN控制器筛选器起始的Bank

    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        //printf("CAN Filter Config Fail!\r\n");
        Error_Handler();
    }

    //printf("CAN Filter Config Success!\r\n");
}

uint8_t expectedData[8] = {0}; // 用于存储预期的数据
uint32_t canrecv_cnt;
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    uint16_t len = 0;
    CAN_RxHeaderTypeDef can_Rx;
    uint8_t recvBuf[8];
    uint8_t usbBuf[200];  // Added 200-byte array for USB virtual serial port print
    
    
    HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &can_Rx, recvBuf);
    
		canrecv_cnt++;
		
		// 数据连续性校验
    uint8_t dataValid = 1; // 假设数据是有效的
    for (int i = 0; i < 8; i++) {
        if (recvBuf[i] != expectedData[i]) {
            dataValid = 0; // 数据不匹配，标记为无效
            break;
        }
    }

    if (dataValid) {
        // 数据有效，更新预期数据
        for (int i = 0; i < 8; i++) {
            expectedData[i]++;
            if (expectedData[i] != 0) {
                break; // 如果当前字节没有溢出，停止更新
            }
        }
    } else {
        // 数据无效，发送错误信息
        len += sprintf((char *)&usbBuf[len], "Data continuity error! Expected: ");
        for (int i = 0; i < 8; i++) {
            len += sprintf((char *)&usbBuf[len], "%02X ", expectedData[i]);
        }
        len += sprintf((char *)&usbBuf[len], "Received: ");
        for (int i = 0; i < 8; i++) {
            len += sprintf((char *)&usbBuf[len], "%02X ", recvBuf[i]);
        }
        len += sprintf((char *)&usbBuf[len], "\r\n");
        CDC_Transmit_FS(usbBuf, len);
    }
	
#define DEBUG_PRINTF 1
#if DEBUG_PRINTF
    if (can_Rx.IDE == CAN_ID_STD)
    {
        len += sprintf((char *)&usbBuf[len], "Standard ID:%#X; ", can_Rx.StdId);
    }
    else if (can_Rx.IDE == CAN_ID_EXT)
    {
        len += sprintf((char *)&usbBuf[len], "Extended ID:%#X; ", can_Rx.ExtId);
    }
   
    if (can_Rx.RTR == CAN_RTR_DATA)
    {
        len += sprintf((char *)&usbBuf[len], "Data Frame; Data:");
       
        for (int i = 0; i < can_Rx.DLC; i++)
        {
            len += sprintf((char *)&usbBuf[len], "%X ", recvBuf[i]);
        }
       
        len += sprintf((char *)&usbBuf[len], "\r\n");
        CDC_Transmit_FS(usbBuf, len);  // Use usbBuf to send data
    }
    else if (can_Rx.RTR == CAN_RTR_REMOTE)
    {
        len += sprintf((char *)&usbBuf[len], "Remote Frame\r\n");
        CDC_Transmit_FS(usbBuf, len);  // Use usbBuf to send data
    }
#endif 
#undef DEBUG_PRINTF
}

/**
  * @brief  发送CAN数据帧
  * @param  id: CAN报文的标识符（标准标识符）
  * @param  data: 指向要发送的数据的指针
  * @param  dlc: 数据长度（0-8）
  * @retval None
  */
void cansend(uint32_t id, uint8_t* data, uint8_t dlc) {
  CAN_TxHeaderTypeDef txHeader;
  uint32_t txMailbox;

  // 设置CAN报文参数
  txHeader.StdId = id;          // 设置标准标识符
  txHeader.ExtId = 0x00;           // 设置扩展标识符
  txHeader.RTR = CAN_RTR_DATA;  // 数据帧
  txHeader.IDE = CAN_ID_STD;    // 标准标识符
  txHeader.DLC = dlc;           // 数据长度

  // 发送CAN数据
  if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, data, &txMailbox) != HAL_OK) {
      while (1);
      // 处理发送错误
  }
}

void send_hex_data() {
  uint8_t data[8];
  data[0] = 0x12;  // 数据的高字节
  data[1] = 0x34;  // 数据的低字节

  cansend(0x125, data, 8);  // 发送数据，ID为0x123，数据为0x 数据长度为2
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
  MX_USB_DEVICE_Init();
  MX_CAN1_Init();
  /* USER CODE BEGIN 2 */

  // CAN 输出通道选择设置
  HAL_GPIO_WritePin(HCAN_RS_EN1_GPIO_Port, HCAN_RS_EN1_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(CHAN_EN_GPIO_Port, CHAN_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CHAN_A0_GPIO_Port, CHAN_A0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(CHAN_A1_GPIO_Port, CHAN_A1_Pin, GPIO_PIN_RESET);

  // OBD 6 -> 7 通道选择
  HAL_GPIO_WritePin(PORT0_EN_GPIO_Port, PORT0_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT0_A0_GPIO_Port, PORT0_A0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PORT0_A1_GPIO_Port, PORT0_A1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT0_A2_GPIO_Port, PORT0_A2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PORT0_A3_GPIO_Port, PORT0_A3_Pin, GPIO_PIN_RESET);

  // OBD 14 -> 15 通道选择
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

	HAL_Delay(500);   
  handle_cantest_welcome();
  
	
	/* Test if Key push-button on STM3210C-EVAL RevC Board is pressed */
  if (1 )
  { 
  	/* Initialise Flash */
  	FLASH_If_Init();
  	/* Execute the IAP driver in order to reprogram the Flash */
    IAP_Init();
    /* Display main menu */
    Main_Menu ();
  }
  /* Keep the user application running */
  else
  {
    /* Test if user code is programmed starting from address "APPLICATION_ADDRESS" */
    if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
    {
      /* Jump to user application */
      JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
      JumpToApplication = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
      JumpToApplication();
    }
  }

  while (1)
  {}
	
  while (1)
  {

    HAL_GPIO_WritePin(LED_CTR_GPIO_Port, LED_CTR_Pin, GPIO_PIN_SET);
    HAL_Delay(500);   
    HAL_GPIO_WritePin(LED_CTR_GPIO_Port, LED_CTR_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
		
#if 0    
		send_hex_data();
		CDC_Transmit_FS("fuck\r\n", 6);   
#endif	
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
  RCC_OscInitStruct.PLL.PLLQ = 5;
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
