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

/* Private typedef -----------------------------------------------------------*/ 
enum {noSession, activeSession, downloadRequested} programmingSessionStatus;

typedef struct
{
	void (*tx_msg_func)(uint32_t id, uint8_t *data, uint8_t len);
} can_uds_t;

/* Private define ------------------------------------------------------------*/ 

/* Private macro -------------------------------------------------------------*/ 

/* Private variables ---------------------------------------------------------*/ 
can_uds_t can_uds = {0};

canMsg TxMessage;
canMsg RxMessage;

volatile uint8_t bootloaderActive;

uint32_t memoryAddress;
uint32_t memorySize;

uint16_t writeCrc;

volatile uint8_t eraseFlag;
volatile uint8_t writeLen;
volatile uint8_t endFlag;
volatile uint8_t blockSequenceCounter;

uint8_t CANRxDataBuf[256];
volatile uint16_t CANRxCurrLen;
volatile uint16_t CANRxCurrWr;
//uint16_t CANRxCurrRd;
uint8_t *CANRxCurrDataPtr;
uint32_t CANRxCurrFlash;

/* Private function prototypes -----------------------------------------------*/ 
void CANProcessPacket(uint8_t *dataBuf, uint8_t len);
void CANAnswer(uint8_t serviceId, uint8_t *dataBuf, uint8_t len);
void CANRxPacket(uint8_t *dataBuf, uint8_t currlen);
inline void CANRxError(void);
void CANSendFC(uint8_t fs, uint8_t bs, uint8_t stmin);

/* Exported functions -*/ 
/* Private functions ---------------------------------------------------------*/ 

void CANProcessPacket(uint8_t *dataBuf, uint8_t len) // Process received packet
{
	if(len == 0)
	{
		CANRxError();
		return;
	}
	CANRxCurrWr = 0;
	CANRxCurrLen = 0;

	uint8_t serviceId = dataBuf[0];
	uint8_t sendBuf[8];
	uint8_t errCode = 0;

	bootloaderActive = 1;

	switch(serviceId)
	{
	case 0x10: // diagnosticSessionControl
		if(dataBuf[1] == 0x02) // programmingSession
		{
			//flash_unlock();
			sendBuf[0] = 0x02;
			sendBuf[1] = 0x0F;
			sendBuf[2] = 0x0F;
			sendBuf[3] = 0x0F;
			sendBuf[4] = 0x0F;
			programmingSessionStatus = activeSession;
			CANAnswer(0x50, sendBuf, 5);
		}else
			errCode = 0x12; // subFunctionNotSupported
		break;
	case 0x11: // ECUReset
		if(dataBuf[1] == 0x01) // HardReset
		{
			sendBuf[0] = 0x01;
			CANAnswer(0x51, sendBuf, 1);
			//iwdg_start();
		}else
			errCode = 0x22; // conditionsNotCorrect
		break;
	case 0x22: // ReadDataByIdentifier
		if(dataBuf[1] == 0xF1 && dataBuf[2] == 0x80) // bootSoftwareIdentificationDataIdentifier
		{
			sendBuf[0] = dataBuf[1];
			sendBuf[1] = dataBuf[2];
			sendBuf[2] = 'B';
			sendBuf[3] = '1';
			sendBuf[4] = '-';
			sendBuf[5] = '0';
			CANAnswer(0x62, sendBuf, 6);
		}else
			errCode = 0x31; // requestOutOfRange
		break;
	case 0x34: // RequestDownload
		if(programmingSessionStatus != activeSession)
		{
			errCode = 0x22; // conditionsNotCorrect
		}else
		{
			//uint8_t dataFormatIdentifier = dataBuf[1];
			//uint8_t addressAndLengthFormatIdentifier = dataBuf[2];
			if((dataBuf[1] == 0x00 || dataBuf[1] == 0x01) && dataBuf[2] == 0x44)
			{
				if(len == 11)
				{
					//needDescramble = dataBuf[1];
					memoryAddress = dataBuf[3]<<24;
					memoryAddress += dataBuf[4]<<16;
					memoryAddress += dataBuf[5]<<8;
					memoryAddress += dataBuf[6];
					memorySize = dataBuf[7]<<24;;
					memorySize += dataBuf[8]<<16;;
					memorySize += dataBuf[9]<<8;;
					memorySize += dataBuf[10];
					
                    if(memoryAddress >= PROG_START_ADDR
                            && memoryAddress <= PROG_END_ADDR
                            && memoryAddress + memorySize <= PROG_END_ADDR)
                    {
                        programmingSessionStatus = downloadRequested;

                        // Erase Flash
                        eraseFlag = 1;
                        CANRxCurrFlash = memoryAddress;
                        writeCrc = 0;

                        sendBuf[0] = 0x10;
                        sendBuf[1] = 0xFA; // Max chunk size
                        blockSequenceCounter = 0x01;
                        CANAnswer(0x74, sendBuf, 2);
                    }else
                    {
                        errCode = 0x31; // requestOutOfRange
                    }
				}else
				{
					errCode = 0x13; // incorrectMessageLengthOrInvalidFormat
				}
			}else
				errCode = 0x31; // requestOutOfRange
		}
		break;
	case 0x36: // TransferData
		if(programmingSessionStatus != downloadRequested)
		{
			errCode = 0x24; // requestSequenceError
		}else
		{
			if(blockSequenceCounter == dataBuf[1])
			{
				// Write to flash
				CANRxCurrDataPtr = &dataBuf[2];
				writeLen = len-2;
			}else
				errCode = 0x73; // wrongBlockSequenceCounter
		}
		break;
	case 0x37: // RequestTransferExit
		if(programmingSessionStatus != downloadRequested)
		{
			errCode = 0x24; // requestSequenceError
		}else
		{
			endFlag = 1;
		}
		break;
	case 0x3E: // TesterPresent
		sendBuf[0] = 0x00;
		CANAnswer(0x7E, sendBuf, 1);
		break;
	default:
		errCode = 0x12; // subFunctionNotSupported
		break;
	}
	if(errCode) // Negative
	{
		sendBuf[0] = serviceId;
		sendBuf[1] = errCode;
		CANAnswer(0x7F, sendBuf, 2);
	}
}

void CANAnswer(uint8_t serviceId, uint8_t *dataBuf, uint8_t len)
{
	if(len>6)
		return;

	TxMessage.Data[0] = len+1;
	TxMessage.Data[1] = serviceId;
	TxMessage.Data[2] = 0x00;
	TxMessage.Data[3] = 0x00;
	TxMessage.Data[4] = 0x00;
	TxMessage.Data[5] = 0x00;
	TxMessage.Data[6] = 0x00;
	TxMessage.Data[7] = 0x00;

	for(int i=2;i<len+2;i++)
		TxMessage.Data[i] = dataBuf[i-2];

	can_uds.tx_msg_func(TxMessage.Id, TxMessage.Data,TxMessage.DLC);
}

void CANRxPacket(uint8_t *dataBuf, uint8_t currlen)
{
	for(int i=0;i<currlen;i++)
	{
		CANRxDataBuf[CANRxCurrWr++] = dataBuf[i];
	}
	if(CANRxCurrWr >= CANRxCurrLen)
		CANProcessPacket(CANRxDataBuf,CANRxCurrLen);
}

inline void CANRxError()
{
	CANRxCurrLen = 0;
}

void CANSendFC(uint8_t fs, uint8_t bs, uint8_t stmin)
{
	TxMessage.Data[0] = 0x30 + (fs & 0xF);
	TxMessage.Data[1] = bs;
	TxMessage.Data[2] = stmin;
	TxMessage.Data[3] = 0x00;
	TxMessage.Data[4] = 0x00;
	TxMessage.Data[5] = 0x00;
	TxMessage.Data[6] = 0x00;
	TxMessage.Data[7] = 0x00;

	can_uds.tx_msg_func(TxMessage.Id, TxMessage.Data,TxMessage.DLC);
}

void can_uds_handle(uint32_t id, uint8_t dlc, uint8_t *data)
{
    // 赋值给 RxMessage 结构体
    RxMessage.Id = id;
    RxMessage.DLC = dlc;
    memcpy(RxMessage.Data, data, dlc);

	static uint16_t RxLen = 0;
	static uint8_t CF_SN = 0;
	static uint8_t sendBuf[8];

	switch((RxMessage.Data[0]) >> 4) // PCIType
	{
	case 2: // ConsecutiveFrame
		if(CF_SN == (RxMessage.Data[0] & 0xF) && CANRxCurrLen)
		{
			CANRxPacket(&RxMessage.Data[1], 7);
			if(++CF_SN>=0x10)
				CF_SN = 0;
		}else
		{
			CANRxError();
		}
		break;
	case 0: // SingleFrame
		RxLen = RxMessage.Data[0];
		CANProcessPacket(&RxMessage.Data[1], RxLen);
		break;
	case 1: // FirstFrame
		RxLen = (RxMessage.Data[0] & 0xF) << 8;
		RxLen += RxMessage.Data[1];
		if(writeLen) // Need wait
		{
			sendBuf[0] = RxMessage.Data[2];
			sendBuf[1] = 0x78;
			CANAnswer(0x7F, sendBuf, 2);
			CANRxError();
			break;
		}
		CANRxCurrLen = RxLen;
		CANRxCurrWr = 0;
		CF_SN = 1;
		CANRxPacket(&RxMessage.Data[2], 6);
		if(CANRxCurrLen > 0xff)
		{
			CANRxError();
			CANSendFC(FC_FS_OVFLW, 0, 0);
		}else
			CANSendFC(FC_FS_CTS, 30, 8);
		break;
	case 3: // FlowControl
		break;
	default:
		CANRxError();
		break;
	}
}

void can_uds_init(void)
{
	can_uds.tx_msg_func = &can_send;
}
/************************ (C) COPYRIGHT Jason *****END OF FILE****/
