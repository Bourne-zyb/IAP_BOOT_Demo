#include "can_user.h"
#include "usbd_cdc_if.h"
#include "can.h"
#include <string.h> 


static uint8_t expectedData[8] = {0}; // 用于存储预期的数据
static uint32_t canrecv_cnt;
static CommandEntry commandTable[] = {
  {"restart", handle_restart},
  {"show", handle_show},
//  {"stop", handle_stop},
  // 继续添加更多命令
};


void handle_restart(void) {
  // 将 cnt 变量清零
  canrecv_cnt = 0;
  memset(expectedData, 0, sizeof(expectedData));
  
  uint8_t usbBuf[50]; // 用于存储要发送的字符串
  uint16_t len = 0;

  // 格式化字符串到 usbBuf
  len += sprintf((char *)&usbBuf[len], "cnt has been reset to 0.\r\n");

  // 使用 CDC_Transmit_FS 发送数据   
  CDC_Transmit_FS(usbBuf, len);
}

void handle_show(void) {
  uint8_t usbBuf[50]; // 用于存储要发送的字符串
  uint16_t len = 0;

  // 格式化字符串到 usbBuf
  len += sprintf((char *)&usbBuf[len], "canrecv_cnt: %lu\r\n", canrecv_cnt);

  // 使用 CDC_Transmit_FS 发送数据
  CDC_Transmit_FS(usbBuf, len);
}

void handle_unknown(void) {
  uint8_t usbBuf[50]; // 用于存储要发送的字符串
  uint16_t len = 0;

  // 格式化字符串到 usbBuf
  len += sprintf((char *)&usbBuf[len], "Unknown command.\r\n");

  // 使用 CDC_Transmit_FS 发送数据
  CDC_Transmit_FS(usbBuf, len);
}

// 查找并执行命令
void execute_command(const char *command) {
  for (int i = 0; i < sizeof(commandTable) / sizeof(CommandEntry); i++) {
      if (strcmp(command, commandTable[i].command) == 0) {
          commandTable[i].handler();
          return;
      }
  }
  handle_unknown(); // 如果找不到命令，执行未知命令处理
}

static void handle_cantest_welcome(void) 
{
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
void can_send(uint32_t id, uint8_t* data, uint8_t dlc) {
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


void can_board_init(void)
{
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

    HAL_Delay(100);   

    handle_cantest_welcome();
}




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
