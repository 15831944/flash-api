#include "stdafx.h"
#include "CAN_FlashupdateMsgHandle.h"
#include "ControlCan.h"

//The third headfiles
#include <stdio.h>
#include <assert.h>

/**********************************************************************
CAN_FlashupdateMsgHandle-----constructor function


Parameters:
Return Value:
Precondition:
Postcondition:
**********************************************************************/
CAN_FlashupdateMsgHandle::CAN_FlashupdateMsgHandle(Blob *solver):Solver(solver)
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
	tx_msg = new CAN_PACKED_PROTOCOL_U[MESSAGE_NUM];
	rx_msg = new CAN_PACKED_PROTOCOL_U[MESSAGE_NUM];

	for (UINT16 i = 0; i < MESSAGE_NUM; ++i) {

		tx_msg[i].Frame = msg_init;
		rx_msg[i].Frame = msg_init;
	
	}

	
	for (BYTE i = 0; i < 0x3F; ++i) {

		FlashUpdateProgress[i] = 0;
		FlashUpdateErrorMsg[i].ereor_cnt = 0;
		FlashUpdateErrorMsg[i].error_state_saved = FLASH_UPDATE_INVALID;
		FlashUpdateErrorMsg[i].receive_done = FALSE;
	}
	BlockCount = 0;
}

/**********************************************************************
~CAN_CfgMsgHandle-----destructor function


Parameters:
Return Value:
Precondition:
Postcondition:
**********************************************************************/
CAN_FlashupdateMsgHandle::~CAN_FlashupdateMsgHandle(VOID)
{
	delete m_pHostModuleItc;
	delete[]rx_msg;
	delete[]tx_msg;
}




/**********************************************************************
FlashUpdateRoutine-----FLAHS UPDATE主程序


Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::FlashUpdateRoutine(VOID)
{

	switch (m_pHostModuleItc->u16FlashupdateStatus)
	{
		//开始升级
		//获取升级对象
	case FLASH_UPDATE_START:

		ParameterRefresh();

		break;

		//发送握手命令并等待握手应答信号
	case SEND_MSG_WAITING_HANDS_RESPOND:

		HandCommProcess();
		break;
		//发送芯片解密命令等待芯片解密应答信号
	case SEND_MSG_WAITING_CHIP_DECODE_RESPOND:

		ChipDecodeProcess();
		break;
		//发送命令等待API版本确认信息
	case SEND_MSG_WAITING_API_VERSION_OK:
		
		VerifyApiVersion();
		break;
		//发送擦除命令
	case SEND_MSG_FLASH_ERASE:
		
		EraseSectorOrderXmit();
		break;
		//等待擦除完毕
	case WAITING_MSG_ERASE_END:

		EraseSectorStatusRecv();
		//请求编程许可等待编程允许
	case SEND_MSG_PROGRAM_PERMIT_WAITING_RESPOND:

		ProgramPermissionGet();
		break;
		//传输BLOCK头
	case SEND_MSG_BLOCK_HEAD:	

		BlockHeadXmit();
		break;
		//等待传输BLOCK头结束标志
	case WAITING_BLOCK_HEAD_TRANSFER_OK:

		BlockHeadRecv();
		break;
		//传输BLOCK数据
	case SEND_BLOCK_DATA:

		BlockDataXmit();

		//等待传输BLOCK数据完成标志
	case WAITING_MSG__BLOCK_DATATRANS_END:

		BlockDataRecv();
		break;

		//传输CHECKSUM
	case SEND_MSG_BLOCK_CHECKSUM:

		BlockCheckSumXmit();
		break;
		//等待BLOCK校验成功标志
	case WAITING_MSG_BLOCK_CHECKSUM_OK:

		BlockCheckSumRecv();
		break;
		//下发编程命令, 
	case SEND_ORDER_PROGRAM:

		BlockProgOrderXmit();
		break;
		//等待DSP回传编程状态
	case WAITING_MSG_PROGRAM_OK:

		BlockProgOrderRecv();
		break;
		//发送FLASH 校验命令,
	case SEND_ORDER_FLASH_VERIFY:

		VerifyXmit();
		break;
		// 等待FLASH 校验完毕消息
	case WAITING_FLASH_VERIFY_OK:

		VerifyRecv();
		break;
		//判断是否还有BLOCK需传输, 有则传输BLOCK头,无则向DSP发送重启命令
	case SEND_NEXT_BLOCK_OR_SEND_DSP_RESTART_MSG_WAITING:

		SendNextBlock_DspRestart();
		break;
		//等待DSP FLASHUPDATE成功标志
	case WAITING_FLAG_FLASHUPDATE_COMPLETED:

		FlashUpdateComplete();
		break;

	case FLASH_UPDATE_SUCCEED:
		
	case FLASH_UPDATE_OVER:

		//状态机复位
		//m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;
		break;
	default:

		//状态机复位
		m_pHostModuleItc->u16FlashupdateStatus = FLASH_UPDATE_INVALID;

		break;

	}


}


INT32 CAN_FlashupdateMsgHandle::ParameterRefresh(VOID)
{
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

		tx_msg[i].Frame = msg_init;
		rx_msg[i].Frame = msg_init;

	}


	for (BYTE i = 0; i < 0x3F; ++i) {

		FlashUpdateProgress[i] = 0;
		FlashUpdateErrorMsg[i].ereor_cnt = 0;
		FlashUpdateErrorMsg[i].error_state_saved = FLASH_UPDATE_INVALID;
		FlashUpdateErrorMsg[i].receive_done = FALSE;
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
	//发握手命令
	TX_MESSAGE_FUNCTION(0, HANDS_COMM_SRVCODE, 4);

	tx_msg->PackedMsg.MsgData[0] = HAND_COMM_QUERY & 0xFF;
	tx_msg->PackedMsg.MsgData[1] = 0xFF & (HAND_COMM_QUERY >> 8);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);


	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;


	for (int i = 0; i < msg_num; ++i) {

		
		if (MESSAGE_FILLTER(HANDS_COMM_SRVCODE))
		{

			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
			//握手成功
			if (HAND_OK_RESPOND == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
				
			}

		}
	}
	//等待解密信息反馈
	MsgErrorProcess(SEND_MSG_WAITING_CHIP_DECODE_RESPOND, FALSE);


	return u16RetrunStatus;
}



INT32 CAN_FlashupdateMsgHandle::ChipDecodeProcess(VOID){


	TX_MESSAGE_FUNCTION(0, CHIP_DECODE_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);


	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;

	for (int i = 0; i < msg_num; ++i) {
		if (MESSAGE_FILLTER(CHIP_DECODE_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//芯片解密OK
			if (CHIP_DECODE_SUCCESS == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
				
			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}
		}
	}
	
	MsgErrorProcess(SEND_MSG_WAITING_API_VERSION_OK, FALSE);

	return u16RetrunStatus;
}



INT32 CAN_FlashupdateMsgHandle::VerifyApiVersion(VOID){


	UINT16 u16RetrunStatus = 0;
	TX_MESSAGE_FUNCTION(0, API_VERSION_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);


	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(API_VERSION_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//API版本核对成功
			if (API_VESION_OK == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;

			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}

		}
	}
	MsgErrorProcess(SEND_MSG_FLASH_ERASE, FALSE);
	return u16RetrunStatus;
}



INT32 CAN_FlashupdateMsgHandle::EraseSectorOrderXmit(VOID){


	TX_MESSAGE_FUNCTION(0, ERASE_SECTOR_SRVCODE, 4);
	*((UINT16*)(tx_msg->PackedMsg.MsgData)) = ERASE_SECTOR_ALL;
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG_ERASE_END;

	return 0;
}


INT32 CAN_FlashupdateMsgHandle::EraseSectorStatusRecv(VOID){


	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	UINT16 u16RetrunStatus = 0;
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(ERASE_SECTOR_SRVCODE)) {
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
			if (ERASE_SUCCESFULL == u16RetrunStatus) {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}
		}

	}
	MsgErrorProcess(SEND_MSG_PROGRAM_PERMIT_WAITING_RESPOND, FALSE);

	return u16RetrunStatus;
}

INT32 CAN_FlashupdateMsgHandle::ProgramPermissionGet(VOID){


	TX_MESSAGE_FUNCTION(0, PROGRAM_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	UINT16 u16RetrunStatus = 0;
	UINT16 msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(PROGRAM_SRVCODE)) {

			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);


			if (PROGRAM_ENABLE == u16RetrunStatus) {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = 0;
			}

			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}

		}
	}
	MsgErrorProcess(SEND_MSG_BLOCK_HEAD, FALSE);

	return u16RetrunStatus;
}



INT32 CAN_FlashupdateMsgHandle::BlockHeadXmit(VOID){



	TX_MESSAGE_FUNCTION(0, BLOCK_HEAD_SRVCODE, 8);
	tx_msg->PackedMsg.MsgData[0] = (BYTE)(Solver->EveryBlockDataNum[BlockCount] & 0x00FF);
	tx_msg->PackedMsg.MsgData[1] = (BYTE)((Solver->EveryBlockDataNum[BlockCount] >> 8) & 0x00FF);

	tx_msg->PackedMsg.MsgData[2] = (BYTE)(Solver->BlockAddress[BlockCount] & 0x00FF);
	tx_msg->PackedMsg.MsgData[3] = (BYTE)((Solver->BlockAddress[BlockCount] >> 8) & 0x00FF);
	tx_msg->PackedMsg.MsgData[4] = (BYTE)((Solver->BlockAddress[BlockCount] >> 16) & 0x00FF);
	tx_msg->PackedMsg.MsgData[5] = (BYTE)((Solver->BlockAddress[BlockCount] >> 24) & 0x00FF);

	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
	m_pHostModuleItc->u16FlashupdateStatus = WAITING_BLOCK_HEAD_TRANSFER_OK;

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::BlockHeadRecv(VOID){


	UINT16 u16RetrunStatus = 0;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_HEAD_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);


			//允许传有效数据,准备发数据信息
			if (BLOCK_HEAD_OK == u16RetrunStatus) {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = 0;
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}

			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_BLOCK_DATA, FALSE);
	return u16RetrunStatus;
}



INT32 CAN_FlashupdateMsgHandle::BlockDataXmit(VOID){



	BYTE *msg_data_ptr = (BYTE*)(Solver->BlockData[BlockCount]);
	UINT16 msg_num = (Solver->EveryBlockDataNum[BlockCount] << 1) / 6;
	for (UINT16 i = 0; i < msg_num + 2; ++i) {

		TX_MESSAGE_FUNCTION(i, BLOCK_DATA_SRVCODE, 8);
		if (i < msg_num) {

			tx_msg[i].PackedMsg.DataLen = 8;
		}
		else if (i == msg_num) {

			tx_msg[i].PackedMsg.DataLen = (Solver->EveryBlockDataNum[BlockCount] << 1) % 6 + 2;
		}
		else {

			tx_msg[i].PackedMsg.DataLen = 2;
		}
		for (UINT16 j = 0; j < 6; ++j) {

			tx_msg[i].PackedMsg.MsgData[j] = *msg_data_ptr;
			msg_data_ptr++;
		}

	}

	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, msg_num + 2);


	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG__BLOCK_DATATRANS_END;

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::BlockDataRecv(VOID) {

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;

	for (int i = 0; i < msg_num; ++i) {
		if (MESSAGE_FILLTER(BLOCK_DATA_SRVCODE)) {
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			// dsp接收到的帧数与下发帧数相同, 则传CHECKSUM
			if (((Solver->EveryBlockDataNum[BlockCount] << 1) / 6 + 1) <= u16RetrunStatus) {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = 0;
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}
		}


	}
	MsgErrorProcess(SEND_MSG_BLOCK_CHECKSUM, FALSE);
	return u16RetrunStatus;
}




INT32 CAN_FlashupdateMsgHandle::BlockCheckSumXmit(VOID){



	TX_MESSAGE_FUNCTION(0, BLOCK_CHECKSUM_SRVCODE, 6)
	tx_msg->PackedMsg.MsgData[0] = (BYTE)(Solver->BlockCheckSum[BlockCount] & 0x00FF);
	tx_msg->PackedMsg.MsgData[1] = (BYTE)((Solver->BlockCheckSum[BlockCount] >> 8) & 0x00FF);
	tx_msg->PackedMsg.MsgData[2] = (BYTE)((Solver->BlockCheckSum[BlockCount] >> 16) & 0x00FF);
	tx_msg->PackedMsg.MsgData[3] = (BYTE)((Solver->BlockCheckSum[BlockCount] >> 24) & 0x00FF);

	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG_BLOCK_CHECKSUM_OK;


	return 0;
}




INT32 CAN_FlashupdateMsgHandle::BlockCheckSumRecv(VOID){


	UINT16 u16RetrunStatus = 0;
	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_CHECKSUM_SRVCODE))
		{

			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			if (CHECK_SUM_SUCCESFUL == u16RetrunStatus) {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = 0;
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_ORDER_PROGRAM, FALSE);

	return u16RetrunStatus;
}




INT32 CAN_FlashupdateMsgHandle::BlockProgOrderXmit(VOID) {

	TX_MESSAGE_FUNCTION(0, BLOCK_PROMG_STATUS_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = WAITING_MSG_PROGRAM_OK;
	return 0;
}




INT32 CAN_FlashupdateMsgHandle::BlockProgOrderRecv(VOID){



	UINT16 u16RetrunStatus = 0;
	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_PROMG_STATUS_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			if (PROGRAM_STATUS_SUCCESS == u16RetrunStatus) {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_ORDER_FLASH_VERIFY, FALSE);

	return u16RetrunStatus;
}




INT32 CAN_FlashupdateMsgHandle::VerifyXmit(VOID) {


	TX_MESSAGE_FUNCTION(0, VERIFY_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
	m_pHostModuleItc->u16FlashupdateStatus = WAITING_FLASH_VERIFY_OK;

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::VerifyRecv(VOID){

	UINT16 u16RetrunStatus = 0;
	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(VERIFY_SRVCODE))
		{
			FlashUpdateProgress[rx_msg[i].PackedMsg.b6SourceMacId] = BlockCount;
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//校验成功,升级成功,反馈给后台TBD
			if (VERIFY_OK == u16RetrunStatus){

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = 0;
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else{

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG;
			}
		}
	}
	MsgErrorProcess(SEND_NEXT_BLOCK_OR_SEND_DSP_RESTART_MSG_WAITING, FALSE);
	
	return u16RetrunStatus;
}



INT32 CAN_FlashupdateMsgHandle::SendNextBlock_DspRestart(VOID) {

	BlockCount++;
	if (BlockCount >= Solver->BlockCount)
	{

		TX_MESSAGE_FUNCTION(0, BLOCK_HEAD_SRVCODE, 8);
		// send 0x00 restart dsp
		tx_msg->PackedMsg.MsgData[0] = 0x00;
		tx_msg->PackedMsg.MsgData[1] = 0x00;

		tx_msg->PackedMsg.MsgData[2] = 0x00;
		tx_msg->PackedMsg.MsgData[3] = 0x00;
		tx_msg->PackedMsg.MsgData[4] = 0x00;
		tx_msg->PackedMsg.MsgData[5] = 0x00;
		VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
		m_pHostModuleItc->u16FlashupdateStatus = WAITING_FLAG_FLASHUPDATE_COMPLETED;

	}

	//还有BLOCK需传输,
	else
	{

		//状态机回到传输下一个BLOCK头
		m_pHostModuleItc->u16FlashupdateStatus = SEND_MSG_BLOCK_HEAD;
	}

	return 0;
}



INT32 CAN_FlashupdateMsgHandle::FlashUpdateComplete(VOID) {

	UINT16 u16RetrunStatus = 0;
	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_HEAD_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			if (FILE_TRANS_END == u16RetrunStatus) {

				m_pHostModuleItc->u16FlashupdateStatus = FLASH_UPDATE_SUCCEED;
			}
		}
	}

	return u16RetrunStatus;
}


void	CAN_FlashupdateMsgHandle::MsgErrorProcess(_FLASHUPDATE_STATUS flash_update_state, BOOL IsNot) {

	UINT16 max_node_num = NodeOffset + 0x10;
	for (UINT16 i = NodeOffset; i < max_node_num; ++i) {

		if (FlashUpdateErrorMsg[i].receive_done == FALSE) {

			if (FlashUpdateErrorMsg[i].ereor_cnt < 123) {

				FlashUpdateErrorMsg[i].ereor_cnt++;

			}
		}

		if ((FlashUpdateErrorMsg[i].receive_done) || (FlashUpdateErrorMsg[i].ereor_cnt > MAX_ERROR_MSG)) {

			MsgReceivedDoneFlagSave |= ((UINT64)1) << i;
			if ((FlashUpdateErrorMsg[i].ereor_cnt > MAX_ERROR_MSG) && (FlashUpdateErrorMsg[i].ereor_cnt < MAX_ERROR_MSG + 2)) {

				MsgErrorSave |= ((UINT64)1) << i;
				FlashUpdateErrorMsg[i].ereor_cnt = 123;
				FlashUpdateErrorMsg[i].error_state_saved = flash_update_state;
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

			FlashUpdateErrorMsg[i].receive_done = FALSE;
			
		}
	}
}



