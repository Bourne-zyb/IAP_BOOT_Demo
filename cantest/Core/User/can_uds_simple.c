/****************************************************************************** 
 * @file    can_uds_simple.c 
 * @brief   This file provides simple UDS functions for CAN bus. 
 * @author  Jason 
 * @version V1.0.0 
 * @date    2025-3 
 * @copyright (c) 2025, All rights reserved. 
 ******************************************************************************/ 

/* Includes ------------------------------------------------------------------*/ 
#include "can_uds_simple.h" 
#include "flash_if.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/ 

/* Private define ------------------------------------------------------------*/ 

/* Private macro -------------------------------------------------------------*/ 

/* Private variables ---------------------------------------------------------*/ 
// 全局缓冲区
static uint8_t uds_data_buffer[UDS_MAX_PAYLOAD_SIZE];
static uint16_t uds_data_length = 0; // 当前接收数据长度
static uint16_t expected_data_length = 0; // 总数据长度
static uint8_t last_seq_number = 0xFF; // 上一个连续帧序号（用于连续性判断）
static programmingSessionStatus_t currentSessionStatus = noSession;
static can_uds_t can_uds = {0};

/* Private function prototypes -----------------------------------------------*/ 
void send_flow_control_frame(FlowControlType type, uint8_t block_size, uint8_t separation_time);
void send_uds_error_response(UDS_ErrorCode error_code);
void send_iso15765_message(uint32_t id, uint8_t *data, uint16_t length);

// 服务处理函数声明
void uds_handle_session_control(uint8_t *data, uint16_t length);     // 0x10 会话控制
void uds_handle_ecu_reset(uint8_t *data, uint16_t length);           // 0x11 ECU重置
void uds_handle_request_download(uint8_t *data, uint16_t length);    // 0x34 请求下载
void uds_handle_transfer_data(uint8_t *data, uint16_t length);       // 0x36 数据传输
void uds_handle_transfer_exit(uint8_t *data, uint16_t length);       // 0x37 传输退出
void uds_handle_routine_control(uint8_t *data, uint16_t length);     // 0x31 例行控制
void send_uds_error_response(UDS_ErrorCode error_code);              // 错误响应函数
void process_uds_service(uint8_t *data, uint16_t length);            // 服务分发函数

/* Private functions ---------------------------------------------------------*/ 
// ISO15765 主处理函数
void can_uds_handle(uint32_t canid, uint8_t *data, uint8_t dlc) {
    // 检查传入的 CAN ID 是否有效
    if (canid != CANID_UPGRADE_TARGET) {
        printf("CAN ID 0x%X is not valid. Ignoring message.\n", canid);
        return;
    }

    // 帧类型解析
    switch (data[0] & 0xF0) {
        case 0x00: { // 单帧 Single Frame
            uint8_t sf_length = data[0] & 0x0F; // 提取数据长度
            memcpy(uds_data_buffer, &data[1], sf_length); // 复制数据到缓冲区
            uds_data_length = sf_length;

            // 调用服务处理函数
            process_uds_service(uds_data_buffer, uds_data_length);
            break;
        }
        case 0x10: { // 首帧 First Frame
            expected_data_length = ((data[0] & 0x0F) << 8) | data[1]; // 提取总数据长度
            uds_data_length = 0; // 重置当前长度
            memcpy(uds_data_buffer, &data[2], dlc - 2); // 复制首帧数据
            uds_data_length += dlc - 2;

            // 发送流控帧 (CTS)
            send_flow_control_frame(FLOW_STATUS_CONTINUE, 00, 0x20); // Block Size = 00(无限制), Separation Time = 5ms
            break;
        }
        case 0x20: { // 连续帧 Consecutive Frame
            uint8_t seq_number = data[0] & 0x0F; // 提取连续帧序号

            // 判断连续性
            if ((last_seq_number != 0xFF) && ((seq_number != ((last_seq_number + 1) & 0x0F)))) {
                printf("Frame sequence error: Expected 0x%X but got 0x%X\n", (last_seq_number + 1) & 0x0F, seq_number);
                send_uds_error_response(UDS_ERROR_TRANSFER_DATA_ERROR); // 发送无效序列错误
                uds_data_length = 0;
                expected_data_length = 0;
                return;
            }

            last_seq_number = seq_number; // 更新序号
            memcpy(uds_data_buffer + uds_data_length, &data[1], dlc - 1); // 累加数据
            uds_data_length += dlc - 1;
            break;
        }
        default: {
            printf("Unsupported Frame Type: 0x%X\n", data[0]);
            break;
        }
    }

    // 检查数据是否接收完整
    if (uds_data_length >= expected_data_length && expected_data_length > 0) {
        process_uds_service(uds_data_buffer, uds_data_length);
        // 清理缓冲区
        uds_data_length = 0;
        expected_data_length = 0;
        last_seq_number = 0xFF; // 重置序号
    }
}

void process_uds_service(uint8_t *data, uint16_t length) 
{
    if (length < 1) {
        DEBUG_PRINT("Error: Data length is insufficient\n");
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE);
        return;
    }

    UDS_ServiceID service_id = (UDS_ServiceID)data[0];

    // 处理服务ID
    switch (service_id) {
        case UDS_SERVICE_DIAGNOSTIC_SESSION_CONTROL: // 0x10 会话控制
            DEBUG_PRINT("Processing Service ID: 0x10 (Diagnostic Session Control)\n");
            uds_handle_session_control(data + 1, length - 1);
            break;

        case UDS_SERVICE_REQUEST_DOWNLOAD: // 0x34 请求下载
            DEBUG_PRINT("Processing Service ID: 0x34 (Request Download)\n");
            if (currentSessionStatus != activeSession) {
                DEBUG_PRINT("Error: Request Download not allowed in current session\n");
                send_uds_error_response(UDS_ERROR_GENERAL_REJECT);
                return;
            }
            uds_handle_request_download(data + 1, length - 1);
            break;

        case UDS_SERVICE_TRANSFER_DATA: // 0x36 数据传输
            DEBUG_PRINT("Processing Service ID: 0x36 (Transfer Data)\n");
            if (currentSessionStatus != downloadRequested) {
                DEBUG_PRINT("Error: Transfer Data not allowed in current session\n");
                send_uds_error_response(UDS_ERROR_GENERAL_REJECT);
                return;
            }
            uds_handle_transfer_data(data + 1, length - 1);
            break;

        case UDS_SERVICE_TRANSFER_EXIT: // 0x37 传输退出
            DEBUG_PRINT("Processing Service ID: 0x37 (Transfer Exit)\n");
            if (currentSessionStatus != downloadRequested) {
                DEBUG_PRINT("Error: Transfer Exit not allowed in current session\n");
                send_uds_error_response(UDS_ERROR_GENERAL_REJECT);
                return;
            }
            uds_handle_transfer_exit(data + 1, length - 1);
            break;

        case UDS_SERVICE_ROUTINE_CONTROL: // 0x31 例行控制
            DEBUG_PRINT("Processing Service ID: 0x31 (Routine Control)\n");
            if (currentSessionStatus != activeSession) {
                DEBUG_PRINT("Error: Routine Control not allowed in current session\n");
                send_uds_error_response(UDS_ERROR_GENERAL_REJECT);
                return;
            }
            uds_handle_routine_control(data + 1, length - 1);
            break;

        default:
            DEBUG_PRINT("Unknown Service ID: 0x%X\n", service_id);
            send_uds_error_response(UDS_ERROR_SERVICE_NOT_SUPPORTED); // 服务不支持错误
            break;
    }
}

// 流控帧发送函数
void send_flow_control_frame(FlowControlType type, uint8_t block_size, uint8_t separation_time) {
    uint8_t flow_control_frame[8] = {0};

    flow_control_frame[0] = type; // 流控帧类型
    flow_control_frame[1] = block_size; // Block Size
    flow_control_frame[2] = separation_time; // Separation Time

    DEBUG_PRINT("Sending Flow Control Frame: Type=0x%X, Block Size=%u, Separation Time=%u\n",
           type, block_size, separation_time);

	send_iso15765_message(CANID_UPGRADE_SENDER, flow_control_frame, 8);
}

// 错误响应函数
void send_uds_error_response(UDS_ErrorCode error_code) {
	// uint8_t error_response[3] = {0};

    // error_response[0] = 0x7F; // 通用否定响应
    // error_response[1] = uds_data_buffer[0]; // 原服务 ID
    // error_response[2] = error_code; // 错误码

	uint8_t error_response[3] = {0x7F, uds_data_buffer[0], error_code}; // 通用否定响应

    DEBUG_PRINT("Sending UDS Error Response: Service ID=0x%X, Error Code=0x%X\n",
                error_response[1], error_code);
	send_iso15765_message(CANID_UPGRADE_SENDER, error_response, sizeof(error_response));
}

// 服务 0x10: 会话控制 (Diagnostic Session Control)
void uds_handle_session_control(uint8_t *data, uint16_t length) {
    if (length < 1) {
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE);
        return;
    }

    uint8_t sub_function = data[0];
    if (sub_function != 0x02) { // 判断是否为编程会话
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE);
        return;
    }

    DEBUG_PRINT("Entering Programming Session (Service ID: 0x10, Sub-function: 0x02)\n");
    currentSessionStatus = activeSession; // 切换到活动会话状态
    uint8_t response[2] = {0x50, data[0]}; // 正响应
    send_iso15765_message(CANID_UPGRADE_SENDER, response, sizeof(response));
}

// 服务 0x11:  (ECU Reset)
void uds_handle_ecu_reset(uint8_t *data, uint16_t length)
{
    if (length < 1) {
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE);
        return;
    }

    uint8_t sub_function = data[0];
    if (sub_function != 0x01) { // 硬重置
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE);    // 校验数据是否为 11 01
        return;
    }

    DEBUG_PRINT("ECU Reset (Service ID: 0x11, Sub-function: 0x01)\n");
    uint8_t response[2] = {0x51, data[0]}; // 正响应
    send_iso15765_message(CANID_UPGRADE_SENDER, response, sizeof(response));

    can_uds.IAP_if->funtionJumpFunction();
}


// 服务 0x31: 例行控制 (Routine Control)
void uds_handle_routine_control(uint8_t *data, uint16_t length) 
{
    save_data_t  rw_data;

    if (currentSessionStatus != activeSession) {
        send_uds_error_response(UDS_ERROR_CONDITIONS_NOT_CORRECT); // 当前状态不支持例行控制
        return;
    }

    if (data[0] != 0x01 || data[1] != 0xFF) {
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE); // 校验数据是否为 31 01 FF 00
        return;
    }
	switch (data[2])
	{
	case 00:
		can_uds.flash_erase_app_func();
		rw_data.header = HEADER;
		rw_data.iap_msg.status = IAP_NO_APP;
		rw_data.iap_msg.version = 0xA0;
		rw_data.iap_msg.transmitMethod = TRANSMIT_METHOD_CAN;
		rw_data.ender = ENDER;
		write_iap_status(&rw_data);
		break;
	case 01:
		if(NEWAPP_VILIBLE == can_uds.IAP_if->funtionCheckFunction())
		{
				rw_data.header = HEADER;
				rw_data.iap_msg.status = IAP_APP_DONE;
				rw_data.iap_msg.version = 0x0A1;
				rw_data.iap_msg.transmitMethod = TRANSMIT_METHOD_CAN;
				rw_data.ender = ENDER;
				write_iap_status(&rw_data);
		}else{
				rw_data.header = HEADER;
				rw_data.iap_msg.status = IAP_NO_APP;
				rw_data.iap_msg.version = 0x0A1;
				rw_data.iap_msg.transmitMethod = TRANSMIT_METHOD_CAN;
				rw_data.ender = ENDER;
				write_iap_status(&rw_data);
				send_uds_error_response(UDS_ERROR_INVALID_FORMAT); 
				return;
		}
		break;
	default:
		send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE); 
		break;
	}
    DEBUG_PRINT("Routine Control Validated (Service ID: 0x31)\n");

    uint8_t response[4] = {0x71, 0x01, 0xFF, data[2]}; // 正响应
    send_iso15765_message(CANID_UPGRADE_SENDER, response, sizeof(response));
}

// 服务 0x34: 请求下载 (Request Download)
// 
void uds_handle_request_download(uint8_t *data, uint16_t length) {
    if (currentSessionStatus != activeSession) {
        send_uds_error_response(UDS_ERROR_CONDITIONS_NOT_CORRECT); // 当前状态不支持下载
        return;
    }

    if (data[0] != 0x00 || data[1] != 0x44) { // 校验格式标识是否为 0x00 0x44
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE);
        return;
    }

    DEBUG_PRINT("Processing Request Download (Service ID: 0x34)\n");
    currentSessionStatus = downloadRequested; // 切换到下载请求状态
    uint8_t response[4] = {0x74, 0x20, UDS_WRITE_BLOCK_SIZE >> 4, UDS_WRITE_BLOCK_SIZE & 0xFF}; // 正响应,告诉最大包为 1024
    send_iso15765_message(CANID_UPGRADE_SENDER, response, sizeof(response));
}

// 服务 0x36: 传输数据 (Transfer Data)
void uds_handle_transfer_data(uint8_t *data, uint16_t length) {
    if (currentSessionStatus != downloadRequested) {
        send_uds_error_response(UDS_ERROR_CONDITIONS_NOT_CORRECT); // 当前状态不支持数据传输
        return;
    }

    DEBUG_PRINT("Processing Transfer Data (Service ID: 0x36, Block Sequence: 0x01)\n");
    
    can_uds.flash_write_func(PROG_START_ADDR + ((data[0] - 1) * UDS_WRITE_BLOCK_SIZE), \
    (uint32_t*)(data + 1), (length - 1) / 4);

    uint8_t response[2] = {0x76, 0x01}; // 正响应
    send_iso15765_message(CANID_UPGRADE_SENDER, response, sizeof(response));
}


// 服务 0x37: 传输退出 (Transfer Exit)
void uds_handle_transfer_exit(uint8_t *data, uint16_t length) {
    if (currentSessionStatus != downloadRequested) {
        send_uds_error_response(UDS_ERROR_CONDITIONS_NOT_CORRECT); // 当前状态不支持传输退出
        return;
    }

    if (length != 0) { // 传输退出不应有额外数据
        send_uds_error_response(UDS_ERROR_REQUEST_OUT_OF_RANGE);
        return;
    }

    DEBUG_PRINT("Processing Transfer Exit (Service ID: 0x37)\n");
    currentSessionStatus = noSession; // 切换到无会话状态
    uint8_t response[1] = {0x77}; // 正响应
    send_iso15765_message(CANID_UPGRADE_SENDER, response, sizeof(response));
}


// 初始化接口函数
void can_uds_init(void) {
    can_uds.tx_msg_func = &can_send;
    can_uds.flash_write_func = &FLASH_If_Write;
    can_uds.flash_erase_app_func = &FLASH_If_Erase_App_Space;
    can_uds.IAP_if = &iapInterface;
}

// 封装发送接口函数
void send_iso15765_message(uint32_t canid, uint8_t *data, uint16_t length) {
    if (length <= 7) {
        // 单帧 (Single Frame)
        uint8_t single_frame[8] = {0};
        single_frame[0] = 0x00 | length; // 帧类型为单帧 (高 4 位为 0x0，低 4 位为数据长度)
        memcpy(&single_frame[1], data, length); // 复制数据
        can_uds.tx_msg_func(canid, single_frame, length + 1);
    } else {
        // 长数据需要多帧传输
        uint8_t first_frame[8] = {0};
        first_frame[0] = 0x10 | ((length >> 8) & 0x0F); // 帧类型为首帧 (高 4 位为 0x1)
        first_frame[1] = length & 0xFF;                // 总长度低 8 位
        memcpy(&first_frame[2], data, 6);              // 首帧最多包含 6 字节数据
        can_uds.tx_msg_func(canid, first_frame, 8);

        // 发送连续帧
        uint16_t remaining_data = length - 6;
        uint8_t *current_data = data + 6;
        uint8_t sequence_number = 1;

        while (remaining_data > 0) {
            uint8_t consecutive_frame[8] = {0};
            consecutive_frame[0] = 0x20 | (sequence_number & 0x0F); // 帧类型为连续帧 (高 4 位为 0x2)
            uint8_t chunk_size = (remaining_data > 7) ? 7 : remaining_data; // 当前帧传输字节数
            memcpy(&consecutive_frame[1], current_data, chunk_size);        // 复制数据
            can_uds.tx_msg_func(canid, consecutive_frame, chunk_size + 1);

            // 更新剩余数据和指针
            remaining_data -= chunk_size;
            current_data += chunk_size;
            sequence_number++;
        }
    }
}

/************************ (C) COPYRIGHT Jason *****END OF FILE****/



