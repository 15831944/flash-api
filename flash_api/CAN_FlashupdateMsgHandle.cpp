#include "stdafx.h"
#include "CAN_FlashupdateMsgHandle.h"
#include "ControlCan.h"
#include <map>
//The third headfiles
#include <stdio.h>
#include <assert.h>

// A simple registry commands.
typedef INT32(CAN_FlashupdateMsgHandle::*BrewFunction)(VOID);
typedef std::map<_FLASHUPDATE_STATUS, BrewFunction> BrewMap;
BrewMap g_brew_map;


#define RegisterFlashUpdateFunction(FLASH_UPDATE_STSTUS, func)				\
	class __Registerer_##func{												\
	 public: /* NOLINT */													\
	  __Registerer_##func() {												\
		g_brew_map[FLASH_UPDATE_STSTUS] = &CAN_FlashupdateMsgHandle::func;  \
	  }																		\
	};																		\
	__Registerer_##func g_registerer_##func;								\

#define RegisterFunction()\
RegisterFlashUpdateFunction(FLASH_UPDATE_INVALID, FlashUpdateInvalid);\
RegisterFlashUpdateFunction(FLASH_UPDATE_START, ParameterRefresh);\
RegisterFlashUpdateFunction(SEND_MSG_WAITING_HANDS_RESPOND, HandCommProcess);\
RegisterFlashUpdateFunction(SEND_MSG_WAITING_CHIP_DECODE_RESPOND, ChipDecodeProcess);\
RegisterFlashUpdateFunction(SEND_MSG_WAITING_API_VERSION_OK, VerifyApiVersion);\
RegisterFlashUpdateFunction(SEND_MSG_FLASH_ERASE, EraseSectorOrderXmit);\
RegisterFlashUpdateFunction(WAITING_MSG_ERASE_END, EraseSectorStatusRecv);\
RegisterFlashUpdateFunction(SEND_MSG_PROGRAM_PERMIT_WAITING_RESPOND, ProgramPermissionGet);\
RegisterFlashUpdateFunction(SEND_MSG_BLOCK_HEAD, BlockHeadXmit);\
RegisterFlashUpdateFunction(WAITING_BLOCK_HEAD_TRANSFER_OK, BlockHeadRecv);\
RegisterFlashUpdateFunction(SEND_BLOCK_DATA, BlockDataXmit);\
RegisterFlashUpdateFunction(WAITING_MSG__BLOCK_DATATRANS_END, BlockDataRecv);\
RegisterFlashUpdateFunction(SEND_MSG_BLOCK_CHECKSUM, BlockCheckSumXmit);\
RegisterFlashUpdateFunction(WAITING_MSG_BLOCK_CHECKSUM_OK, BlockCheckSumRecv);\
RegisterFlashUpdateFunction(SEND_ORDER_PROGRAM, BlockProgOrderXmit);\
RegisterFlashUpdateFunction(WAITING_MSG_PROGRAM_OK, BlockProgOrderRecv);\
RegisterFlashUpdateFunction(SEND_ORDER_FLASH_VERIFY, VerifyXmit);\
RegisterFlashUpdateFunction(WAITING_FLASH_VERIFY_OK, VerifyRecv);\
RegisterFlashUpdateFunction(SEND_NEXT_BLOCK_OR_SEND_DSP_RESTART_MSG_WAITING, SendNextBlock_DspRestart);\
RegisterFlashUpdateFunction(WAITING_FLAG_FLASHUPDATE_COMPLETED, FlashUpdateComplete);\
RegisterFlashUpdateFunction(FLASH_UPDATE_SUCCEED, FlashUpdateSucceed);\
RegisterFlashUpdateFunction(FLASH_UPDATE_OVER, FlashUpdateOver);\
/**********************************************************************
CAN_FlashupdateMsgHandle-----constructor function


Parameters:
Return Value:
Precondition:
Postcondition:
**********************************************************************/
CAN_FlashupdateMsgHandle::CAN_FlashupdateMsgHandle(const Blob &solver):Solver(solver)
{
	m_ucMsgClass = CAN_RESERVED_CLASS;
	m_pHostModuleItc = new _HOST_MODULE_ITC_T;
	m_pHostModuleItc->u16FlashupdateStatus = FLASH_UPDATE_INVALID;

	VCI_CAN_OBJ msg_init;
	msg_init.DataLen = 0;
	msg_init.ExternFlag = 0;
	msg_init.ID = 0;
	msg_init.RemoteFlag = 0;
	msg_init.Reserved[0] = 0;
	msg_init.Reserved[1] = 0;
	msg_init.Reserved[2] = 0;
	msg_init.SendType = 0;
	msg_init.TimeFlag = 0;
	msg_init.Data[0] = 0;
	msg_init.Data[1] = 0;
	msg_init.Data[2] = 0;
	msg_init.Data[3] = 0;
	msg_init.Data[4] = 0;
	msg_init.Data[5] = 0;
	msg_init.Data[6] = 0;
	msg_init.Data[7] = 0;
	TXMessage = new CAN_PACKED_PROTOCOL_U[MESSAGE_NUM*100];
	RXMessage = new CAN_PACKED_PROTOCOL_U[MESSAGE_NUM];

	for (UINT16 i = 0; i < MESSAGE_NUM; ++i) {


		RXMessage[i].Frame = msg_init;
	}
	for (UINT16 j = 0; j < 50000; ++j) {

		TXMessage[j].Frame = msg_init;
	}
	
	for (BYTE i = 0; i < 0x3F; ++i) {

		FlashUpdateProgress[i] = 0;
		FlashUpdateErrorMsg[i].ErrorCnt = 0;
		FlashUpdateErrorMsg[i].ErrorStateSaved = FLASH_UPDATE_INVALID;
		FlashUpdateErrorMsg[i].ReceiveDone = FALSE;
	}
	BlockCount = 0;
	BootLoaderCount = 0;
	RegisterFunction();
}


/**********************************************************************
~CAN_CfgMsgHandle-----destructor function
**********************************************************************/
CAN_FlashupdateMsgHandle::~CAN_FlashupdateMsgHandle(VOID){

	delete m_pHostModuleItc;
	delete[]RXMessage;
	delete[]TXMessage;
}




/**********************************************************************
FlashUpdateRoutine-----FLAHS UPDATEÖ÷³ÌÐò
**********************************************************************/


VOID CAN_FlashupdateMsgHandle::GetFlashUpdateRoutine(VOID)
{
	if (m_pHostModuleItc->u16FlashupdateStatus) {

		(this->*(g_brew_map[m_pHostModuleItc->u16FlashupdateStatus]))();
	}
	else {

		for (BrewMap::iterator it = g_brew_map.begin();
			it != g_brew_map.end(); ++it) {
			
		}
	}
}
INT32 CAN_FlashupdateMsgHandle::FlashUpdateInvalid(VOID) {

	return 0;
}
INT32 CAN_FlashupdateMsgHandle::FlashUpdateSucceed(VOID) {

	return 0;
}
INT32 CAN_FlashupdateMsgHandle::FlashUpdateOver(VOID) {

	return 0;
}
INT32 CAN_FlashupdateMsgHandle::ParameterRefresh(VOID)
{
	BootLoaderCount = 0;
	MsgReceivedDoneFlagSave = 0;
	MsgErrorSave = 0;
	NodeSelect = 0;

	m_ucMsgClass = CAN_RESERVED_CLASS;
	m_pHostModuleItc->u16FlashupdateStatus = FLASH_UPDATE_INVALID;

	VCI_CAN_OBJ msg_init;
	msg_init.DataLen = 0;
	msg_init.ExternFlag = 0;
	msg_init.ID = 0;
	msg_init.RemoteFlag = 0;
	msg_init.Reserved[0] = 0;
	msg_init.Reserved[1] = 0;
	msg_init.Reserved[2] = 0;
	msg_init.SendType = 0;
	msg_init.TimeFlag = 0;
	msg_init.Data[0] = 0;
	msg_init.Data[1] = 0;
	msg_init.Data[2] = 0;
	msg_init.Data[3] = 0;
	msg_init.Data[4] = 0;
	msg_init.Data[5] = 0;
	msg_init.Data[6] = 0;
	msg_init.Data[7] = 0;

	for (UINT16 i = 0; i < MESSAGE_NUM; ++i) {

		TXMessage[i].Frame = msg_init;
		RXMessage[i].Frame = msg_init;

	}


	for (BYTE i = 0; i < 0x3F; ++i) {

		FlashUpdateProgress[i] = 0;
		FlashUpdateErrorMsg[i].ErrorCnt = 0;
		FlashUpdateErrorMsg[i].ErrorStateSaved = FLASH_UPDATE_INVALID;
		FlashUpdateErrorMsg[i].ReceiveDone = FALSE;
	}
	BlockCount = 0;


	NodeSelect = (m_u16UpdaingNodeAdd == 0x3F) ?
		(UINT64)0xFFFF : ((UINT64)0x0001L << (Module_number - 1));
	NodeSelect = (m_u16UpdaingNodeAdd == 0x0F) ?
		(UINT64)0x8000 : NodeSelect;
	m_pHostModuleItc->u16FlashupdateStatus = SEND_MSG_WAITING_HANDS_RESPOND;
	//m_u16UpdaingNodeAdd = 0x10;
	m_ucMsgClass = CAN_RESERVED_CLASS;
	m_pHostModuleItc->u16FlashUpdateKernelFlag = 0x95;

	return 0;
}


INT32 CAN_FlashupdateMsgHandle::HandCommProcess(VOID){
	// Send shake hand command
	TX_MESSAGE_FUNCTION(0, HANDS_COMM_SRVCODE, 4);

	TXMessage->PackedMsg.MsgData[0] = HAND_COMM_QUERY & 0xFF;
	TXMessage->PackedMsg.MsgData[1] = 0xFF & (HAND_COMM_QUERY >> 8);
	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);

	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);
	
	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	UINT16 return_status = 0;


	for (DWORD i = 0; i < msg_num; ++i) {

		
		if (MESSAGE_FILLTER(HANDS_COMM_SRVCODE))
		{

			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);
			//hand succeed
			if (HAND_OK_RESPOND == return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}

		}
	}
	MsgErrorProcess(SEND_MSG_WAITING_CHIP_DECODE_RESPOND, FALSE);

	return return_status;
}



INT32 CAN_FlashupdateMsgHandle::ChipDecodeProcess(VOID){


	TX_MESSAGE_FUNCTION(0, CHIP_DECODE_SRVCODE, 2);
	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);


	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);
	
	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	UINT16 return_status = 0;

	for (DWORD i = 0; i < msg_num; ++i) {
		if (MESSAGE_FILLTER(CHIP_DECODE_SRVCODE))
		{
			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);

		
			if (CHIP_DECODE_SUCCESS == return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}
			else {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}
	}
	
	MsgErrorProcess(SEND_MSG_WAITING_API_VERSION_OK, FALSE);

	return return_status;
}



INT32 CAN_FlashupdateMsgHandle::VerifyApiVersion(VOID){


	UINT16 return_status = 0;
	TX_MESSAGE_FUNCTION(0, API_VERSION_SRVCODE, 2);
	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);

	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);
	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(API_VERSION_SRVCODE)){

			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);

			if (API_VESION_OK == return_status){

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}
			else {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_MSG_FLASH_ERASE, FALSE);
	return return_status;
}



INT32 CAN_FlashupdateMsgHandle::EraseSectorOrderXmit(VOID){


	TX_MESSAGE_FUNCTION(0, ERASE_SECTOR_SRVCODE, 4);
	*((UINT16*)(TXMessage->PackedMsg.MsgData)) = ERASE_SECTOR_ALL;
	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG_ERASE_END;

	return 0;
}


INT32 CAN_FlashupdateMsgHandle::EraseSectorStatusRecv(VOID){


	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);
	UINT16 return_status = 0;
	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(ERASE_SECTOR_SRVCODE)) {
			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);
			if (ERASE_SUCCESFULL == return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}
			else {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}

	}
	MsgErrorProcess(SEND_MSG_PROGRAM_PERMIT_WAITING_RESPOND, FALSE);

	return return_status;
}

INT32 CAN_FlashupdateMsgHandle::ProgramPermissionGet(VOID){


	TX_MESSAGE_FUNCTION(0, PROGRAM_SRVCODE, 2);
	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);

	UINT16 return_status = 0;
	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);

	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(PROGRAM_SRVCODE)) {

			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);


			if (PROGRAM_ENABLE == return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = 0;
			}

			else {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}

		}
	}
	MsgErrorProcess(SEND_MSG_BLOCK_HEAD, FALSE);

	return return_status;
}



INT32 CAN_FlashupdateMsgHandle::BlockHeadXmit(VOID){



	TX_MESSAGE_FUNCTION(0, BLOCK_HEAD_SRVCODE, 8);
	TXMessage->PackedMsg.MsgData[0] = (BYTE)(Solver.EveryBlockDataNum[BlockCount] & 0x00FF);
	TXMessage->PackedMsg.MsgData[1] = (BYTE)((Solver.EveryBlockDataNum[BlockCount] >> 8) & 0x00FF);

	TXMessage->PackedMsg.MsgData[2] = (BYTE)(Solver.BlockAddress[BlockCount] & 0x00FF);
	TXMessage->PackedMsg.MsgData[3] = (BYTE)((Solver.BlockAddress[BlockCount] >> 8) & 0x00FF);
	TXMessage->PackedMsg.MsgData[4] = (BYTE)((Solver.BlockAddress[BlockCount] >> 16) & 0x00FF);
	TXMessage->PackedMsg.MsgData[5] = (BYTE)((Solver.BlockAddress[BlockCount] >> 24) & 0x00FF);

	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);
	m_pHostModuleItc->u16FlashupdateStatus = WAITING_BLOCK_HEAD_TRANSFER_OK;

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::BlockHeadRecv(VOID){


	UINT16 return_status = 0;

	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);

	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_HEAD_SRVCODE))
		{
			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);


			// Block head transmit ok, ready to transmit block data
			if (BLOCK_HEAD_OK == return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = 0;
				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}

			// received BlockHead transmit Error, and give up this module, do not process it 
			else {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_BLOCK_DATA, FALSE);
	return return_status;
}



INT32 CAN_FlashupdateMsgHandle::BlockDataXmit(VOID){

	BYTE *msg_data_ptr = (BYTE*)(Solver.BlockData[BlockCount]);
	DWORD msg_num = (Solver.EveryBlockDataNum[BlockCount] << 1) / 6;
	for (UINT16 i = 0; i < msg_num + 2; ++i) {

		TX_MESSAGE_FUNCTION(i, BLOCK_DATA_SRVCODE, 8);
		if (i < msg_num) {

			TXMessage[i].PackedMsg.DataLen = 8;
		}
		else if (i == msg_num) {

			TXMessage[i].PackedMsg.DataLen = (Solver.EveryBlockDataNum[BlockCount] << 1) % 6 + 2;
		}
		else {

			TXMessage[i].PackedMsg.DataLen = 2;
		}
		for (UINT16 j = 0; j < 6; ++j) {

			TXMessage[i].PackedMsg.MsgData[j] = *msg_data_ptr;
			msg_data_ptr++;
		}

	}

	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, msg_num + 2);


	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG__BLOCK_DATATRANS_END;

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::BlockDataRecv(VOID) {

	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);

	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	UINT16 return_status = 0;

	for (DWORD i = 0; i < msg_num; ++i) {
		if (MESSAGE_FILLTER(BLOCK_DATA_SRVCODE)) {
			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);

			// dsp received msg num equal to transmit num, and transmit Block CHECKSUM
			if (((Solver.EveryBlockDataNum[BlockCount] << 1) / 6 + 1) <= return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = 0;
				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}
			// do not process this module
			else {

				//FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_MSG_BLOCK_CHECKSUM, FALSE);
	return return_status;
}




INT32 CAN_FlashupdateMsgHandle::BlockCheckSumXmit(VOID){



	TX_MESSAGE_FUNCTION(0, BLOCK_CHECKSUM_SRVCODE, 6)
	TXMessage->PackedMsg.MsgData[0] = (BYTE)(Solver.BlockCheckSum[BlockCount] & 0x00FF);
	TXMessage->PackedMsg.MsgData[1] = (BYTE)((Solver.BlockCheckSum[BlockCount] >> 8) & 0x00FF);
	TXMessage->PackedMsg.MsgData[2] = (BYTE)((Solver.BlockCheckSum[BlockCount] >> 16) & 0x00FF);
	TXMessage->PackedMsg.MsgData[3] = (BYTE)((Solver.BlockCheckSum[BlockCount] >> 24) & 0x00FF);

	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG_BLOCK_CHECKSUM_OK;


	return 0;
}




INT32 CAN_FlashupdateMsgHandle::BlockCheckSumRecv(VOID){


	UINT16 return_status = 0;
	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);

	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_CHECKSUM_SRVCODE))
		{

			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);
			// 
			if (CHECK_SUM_SUCCESFUL == return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = 0;
				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}
			else {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_ORDER_PROGRAM, FALSE);

	return return_status;
}




INT32 CAN_FlashupdateMsgHandle::BlockProgOrderXmit(VOID) {

	TX_MESSAGE_FUNCTION(0, BLOCK_PROMG_STATUS_SRVCODE, 2);
	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG_PROGRAM_OK;
	return 0;
}




INT32 CAN_FlashupdateMsgHandle::BlockProgOrderRecv(VOID){



	UINT16 return_status = 0;
	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);

	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_PROMG_STATUS_SRVCODE))
		{
			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);
			// received dsp xmit message,if dsp respond program status success,
			// transmit order flash verify
			if (PROGRAM_STATUS_SUCCESS == return_status) {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}
			// this module do not process
			else {

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_ORDER_FLASH_VERIFY, FALSE);

	return return_status;
}




INT32 CAN_FlashupdateMsgHandle::VerifyXmit(VOID) {


	TX_MESSAGE_FUNCTION(0, VERIFY_SRVCODE, 2);
	VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);
	m_pHostModuleItc->u16FlashupdateStatus = WAITING_FLASH_VERIFY_OK;

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::VerifyRecv(VOID){

	UINT16 return_status = 0;
	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);
	
	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(VERIFY_SRVCODE))
		{
			FlashUpdateProgress[RXMessage[i].PackedMsg.b6SourceMacId] = BlockCount;
			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);

			//Verify ok, waiting to send dsp restart order, or send next block
			if (VERIFY_OK == return_status){

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = 0;
				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ReceiveDone = TRUE;
			}
			else{

				FlashUpdateErrorMsg[RXMessage[i].PackedMsg.b6SourceMacId].ErrorCnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_NEXT_BLOCK_OR_SEND_DSP_RESTART_MSG_WAITING, FALSE);
	
	return return_status;
}



INT32 CAN_FlashupdateMsgHandle::SendNextBlock_DspRestart(VOID) {

	BlockCount++;
	if (BlockCount >= Solver.BlockCount)
	{

		TX_MESSAGE_FUNCTION(0, BLOCK_HEAD_SRVCODE, 8);
		// send 0x00 restart dsp
		TXMessage->PackedMsg.MsgData[0] = 0x00;
		TXMessage->PackedMsg.MsgData[1] = 0x00;

		TXMessage->PackedMsg.MsgData[2] = 0x00;
		TXMessage->PackedMsg.MsgData[3] = 0x00;
		TXMessage->PackedMsg.MsgData[4] = 0x00;
		TXMessage->PackedMsg.MsgData[5] = 0x00;
		VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, 1);
		m_pHostModuleItc->u16FlashupdateStatus = WAITING_FLAG_FLASHUPDATE_COMPLETED;

	}

	//Send Next Block
	else
	{

		m_pHostModuleItc->u16FlashupdateStatus = SEND_MSG_BLOCK_HEAD;
	}

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::FlashUpdateComplete(VOID) {

	UINT16 return_status = 0;
	DWORD msg_num = VCI_GetReceiveNum(DeviceType, DeviceInd, CanInd);

	CHECK_RX_MESSAGE_NUM();
	VCI_Receive(DeviceType, DeviceInd, CanInd, &(RXMessage->Frame), msg_num, 1);

	for (DWORD i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_HEAD_SRVCODE))
		{
			return_status = *(UINT16 *)(RXMessage[i].PackedMsg.MsgData);

			// DSP respond message, and Flashupdate Succeed
			if (FILE_TRANS_END == return_status) {

				m_pHostModuleItc->u16FlashupdateStatus = FLASH_UPDATE_SUCCEED;
			}
		}
	}

	return return_status;
}


void	CAN_FlashupdateMsgHandle::MsgErrorProcess(_FLASHUPDATE_STATUS flash_update_state, BOOL IsNot) {

	const UINT16 max_node_num = NodeOffset + 0x10;
	for (UINT16 i = NodeOffset; i < max_node_num; ++i) {

		if (FlashUpdateErrorMsg[i].ReceiveDone == FALSE) {

			if (FlashUpdateErrorMsg[i].ErrorCnt < 123) {

				FlashUpdateErrorMsg[i].ErrorCnt++;

			}
		}

		if ((FlashUpdateErrorMsg[i].ReceiveDone) || (FlashUpdateErrorMsg[i].ErrorCnt > MAX_ERROR_MSG)) {

			MsgReceivedDoneFlagSave |= ((UINT64)1) << i;
			if ((FlashUpdateErrorMsg[i].ErrorCnt > MAX_ERROR_MSG) && (FlashUpdateErrorMsg[i].ErrorCnt < MAX_ERROR_MSG + 2)) {

				MsgErrorSave |= ((UINT64)1) << i;
				FlashUpdateErrorMsg[i].ErrorCnt = 123;
				FlashUpdateErrorMsg[i].ErrorStateSaved = flash_update_state;
			}
		}
	}



	if ((MsgReceivedDoneFlagSave & (NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		m_pHostModuleItc->u16FlashupdateStatus = flash_update_state;
		MsgReceivedDoneFlagSave = (IsNot == FALSE) ? 0 : MsgReceivedDoneFlagSave;

		// if ALL of the waiting flash update module error, quit flashupdate at once
		if ((MsgErrorSave & (NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

			m_pHostModuleItc->u16FlashupdateStatus = FLASH_UPDATE_OVER;
		}
		// this state finish, refresh receive done state
		for (UINT16 i = NodeOffset; i < max_node_num; ++i) {

			FlashUpdateErrorMsg[i].ReceiveDone = FALSE;
			
		}
	}
}

// BOOT Loader Message Xmit
VOID CAN_FlashupdateMsgHandle::GetBootLoaderRoutine(VOID) {

	BootMsgPtr = (BYTE*)(Solver.BootLoaderFile);
	//DWORD msg_num = (Solver.BootFileCount)/2;
	
	if (BootLoaderCount > (Solver.BootFileCount)) {

		//m_pHostModuleItc->u16FlashupdateStatus = FLASH_UPDATE_SUCCEED;
		return;
	}
	const INT32 remain_tx_msg_num = Solver.BootFileCount - BootLoaderCount;
	if (BootLoaderCount > 97000) {
	
		int a = 0;
	}
	BootMsgPtr = BootMsgPtr + BootLoaderCount;
	INT32 tx_msg_num = (remain_tx_msg_num > 1000) ? 1000 : remain_tx_msg_num;
	for (UINT16 i = 0; i < tx_msg_num/2; ++i) {

		TXMessage[i].Frame.RemoteFlag = 0;			
		TXMessage[i].Frame.ExternFlag = 0;				
		TXMessage[i].Frame.ID = 0x0001;
		TXMessage[i].Frame.DataLen = 2;

		TXMessage[i].Frame.Data[0] = *BootMsgPtr;
		++BootMsgPtr;
		TXMessage[i].Frame.Data[1] = *BootMsgPtr;
		++BootMsgPtr;
	}
	
	const DWORD tx_wrong_num = VCI_Transmit(DeviceType, DeviceInd, CanInd, &TXMessage->Frame, tx_msg_num/2);
	
	BootLoaderCount = BootLoaderCount + 1000;

	return;
}

