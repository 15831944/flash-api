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
CAN_FlashupdateMsgHandle::CAN_FlashupdateMsgHandle(VOID)
{
	m_ucMsgClass = CAN_RESERVED_CLASS;
	m_pHostModuleItc = new _HOST_MODULE_ITC_T;
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;

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
		FlashUpdateErrorMsg[i].error_state_saved = STATUS_FLASH_UPDATE_INVALID;
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
	case STATUS_FLASH_START:

		ParameterRefresh();
		NodeSelect = (m_u16UpdaingNodeAdd == 0x3F) ?
					 (UINT64)0xFFFF : ((UINT64)0x0001L << (Module_number - 1));
		NodeSelect = (m_u16UpdaingNodeAdd == 0x0F) ?
					 (UINT64)0x8000 : NodeSelect;
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_SELCET_NODE;
		//m_u16UpdaingNodeAdd = 0x10;
		m_ucMsgClass = CAN_RESERVED_CLASS;
		m_pHostModuleItc->u16FlashUpdateKernelFlag = 0x95;
		break;

		//对指定节点开始升级
		//首先发握手命令
	case STATUS_SELCET_NODE:

		HandCommXmitFcb();
		break;

		//等待握手应答信号
	case STATUS_WAITING_HANDS_RESPOND:
		//DO NOTHING
		HandCommXmitFcb();
		HandCommRecvChipDecodeXmit();
		break;
		//等待芯片解密信号, 索要API版本信息
	case STATUS_WAITING_CHIP_DECODE:
		ChipDecodeRecvFcb();
		break;
		//等待API版本确认信息
	case STATUS_WAITING_API_VERSION:
		
		ApiVersionRecvFcb();
		break;
		//API VERSION OK
	case STATUS_API_OK:
		//下发擦除命令
		EraseSectorXmitFcb();

		break;
		//等待擦除命令应答
	case STATUS_FLASH_ERASE_GOING:
		EraseSectorRecvFcb();
		break;
		//擦除成功
	case STATUS_FLASH_ERASED:
		//下发编程请求命令
		ProgramXmitFcb();

		break;
		//等待编程允许状态
	case STATUS_PROGRAM_PERMIT_WAITING:
		ProgramRecvFcb();
		break;
		//编程允许
	case STATUS_PROGRAM_ENABLE:		
		//下发BLOCK头
		BlockHeadXmitFcb();
		break;
		//头BLOCK传输中
	case STATUS_BLOCK_HEAD_WAITING:
		BlockHeadRecvFcb();
		break;
		//下一个BLOCK头
	case STATUS_NEXT_BLOCK_HEAD:
		//下发BLOCK头
		BlockHeadXmitFcb();
		break;

		//头传输结束,开始传输数据
	case STATUS_BLOCK_HEAD_OK:

		BlockDataXmitFcb();

		//BLOCK数据传输过程中
	case STATUS_BLOCK_DATATRANS_WAITING:

		BlockDataRecvFcb();
		break;

		//BLOCK数据传输结束, 下发校验命令
	case STATUS_BLOCK_DATATRANS_END:

		BlockChecksumXmitFcb();
		break;
		//block校验中
	case STATUS_BLOCK_CHECKSUM_WAITING:

		BlockChecksumRecvFcb();
		break;
		//BLOCK校验OK, 下发编程命令
	case STATUS_BLOCK_CHECKSUM_OK:

		BlockProgStatusXmitFcb();
		break;
		//编程状态
	case STATUS_BLOCK_PROGRAM_WAITING:
		
		BlockProgStatusRecvFcb();
		break;

		//编程完毕
	case STATUS_BLOCK_PROGRAM_COMPLETE:

		break;
		//文件传输结束
	case STATUS_FILE_TRANS_END:

		FlashUpdateCompleteRecv();
		break;

		//校验中
	case STATUS_FLASH_VERIFYING:

		VerifyRecvFcb();
		break;
		//校验完毕
	case STATUS_FLASH_UPDATE_OVER:


		//状态机复位
		//m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;
		break;
	default:


		//状态机复位
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;

		break;

	}


}


/**********************************************************************
HandCommXmitFcb-----握手信号下发

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::HandCommXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	//发握手命令
	TX_MESSAGE_FUNCTION(0, HANDS_COMM_SRVCODE, 4);

	tx_msg->PackedMsg.MsgData[0] = HAND_COMM_QUERY & 0xFF;
	tx_msg->PackedMsg.MsgData[1] = 0xFF & (HAND_COMM_QUERY >> 8);

	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	//等待握手应答信号
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_HANDS_RESPOND;

	return ucErrCode;
}

/**********************************************************************
HandCommRecvFcb-----握手信号接收

Parameters:		=0xaa表示握手成功

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::HandCommRecvChipDecodeXmit(VOID)
{
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
	//等待解密信息反馈,启动等待延时TBD
	MsgErrorProcess(STATUS_WAITING_CHIP_DECODE, TRUE);
	//解密命令
	if ((MsgReceivedDoneFlagSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		MsgReceivedDoneFlagSave = 0;

		TX_MESSAGE_FUNCTION(0, CHIP_DECODE_SRVCODE, 2);
		VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	}

	return u16RetrunStatus;
}


/**********************************************************************
ChipDecodeXmitFcb-----解密命令下发--保留

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ChipDecodeXmitFcb(VOID)
{

	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;


	//	FlashupdateTaskReset();

	return ucErrCode;
}

/**********************************************************************
ChipDecodeRecvFcb-----解密信号接收

Parameters:		=0xaa表示握手成功

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ChipDecodeRecvFcb(VOID)
{

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

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG + 1;
			}
		}
	}
	//等待解密信息反馈,启动等待延时TBD
	MsgErrorProcess(STATUS_WAITING_API_VERSION, TRUE);
	//下发获取API版本信息
	if ((MsgReceivedDoneFlagSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		MsgReceivedDoneFlagSave = 0;

		TX_MESSAGE_FUNCTION(0, API_VERSION_SRVCODE, 2);
		VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	}
	return u16RetrunStatus;
}


/**********************************************************************
ApiVersionXmitFcb-----API版本核对命令下发--保留

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ApiVersionXmitFcb(VOID)
{

	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	return ucErrCode;
}

/**********************************************************************
ApiVersionRecvFcb-----API版本核对信号接收

Parameters:		=0x6C表示版本匹配,允许擦除
=14 版本不匹配

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ApiVersionRecvFcb(VOID)
{
	UINT16 u16RetrunStatus = 0;

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

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG + 1;
			}

		}
	}
	MsgErrorProcess(STATUS_API_OK, FALSE);
	return u16RetrunStatus;
}


/**********************************************************************
EraseSectorXmitFcb-----扇区擦除命令下发

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::EraseSectorXmitFcb(VOID)
{

	TX_MESSAGE_FUNCTION(0, ERASE_SECTOR_SRVCODE, 4);

	*((UINT16*)(tx_msg->PackedMsg.MsgData)) = ERASE_SECTOR_ALL;
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
	//下发擦除命令,等待擦除信息反馈
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_ERASE_GOING;

	return 0;
}

/**********************************************************************
EraseSectorRecvFcb-----扇区擦除命令反馈

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::EraseSectorRecvFcb(VOID)
{
	UINT16 u16RetrunStatus = 0;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(ERASE_SECTOR_SRVCODE)){
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
			if (ERASE_SUCCESFULL == u16RetrunStatus){

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG + 1;
			}
		}

	}
	MsgErrorProcess(STATUS_FLASH_ERASED, FALSE);
	return u16RetrunStatus;
}

/**********************************************************************
ProgramXmitFcb-----编程命令下发

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ProgramXmitFcb(VOID)
{

	TX_MESSAGE_FUNCTION(0, PROGRAM_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_PROGRAM_PERMIT_WAITING;

	return 0;
}

/**********************************************************************
ProgramRecvFcb-----编程允许命令反馈

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ProgramRecvFcb(VOID)
{
	UINT16 u16RetrunStatus = 0;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(PROGRAM_SRVCODE)){

			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);


			if (PROGRAM_ENABLE == u16RetrunStatus){

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}

			else{

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG + 1;
			}

		}
	}
	MsgErrorProcess(STATUS_PROGRAM_ENABLE, FALSE);
	return u16RetrunStatus;
}

/**********************************************************************
VerifyXmitFcb-----校验命令下发

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::VerifyXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;



	return ucErrCode;
}

/**********************************************************************
VerifyRecvFcb-----校验命令反馈

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::VerifyRecvFcb(VOID)
{
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


				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else{

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG + 1;
			}
		}
	}
	MsgErrorProcess(STATUS_FLASH_VERIFYING, TRUE);
	if ((MsgReceivedDoneFlagSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		MsgReceivedDoneFlagSave = 0;
		//当前BLOCK号加1
		BlockCount++;
		if (BlockCount >= BlockAmount)
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
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FILE_TRANS_END;

		}

		//还有BLOCK需传输,
		else
		{

			//状态机回到传输下一个BLOCK头
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
		}
	}
	return 0;
}


/**********************************************************************
BlockHeadXmitFcb-----下传BLOCK大小及地址

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockHeadXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	TX_MESSAGE_FUNCTION(0, BLOCK_HEAD_SRVCODE, 8);
	tx_msg->PackedMsg.MsgData[0] = (BYTE)(EveryBlockDataNum[BlockCount] & 0x00FF);
	tx_msg->PackedMsg.MsgData[1] = (BYTE)((EveryBlockDataNum[BlockCount] >> 8) & 0x00FF);

	tx_msg->PackedMsg.MsgData[2] = (BYTE)(BlockAddress[BlockCount] & 0x00FF);
	tx_msg->PackedMsg.MsgData[3] = (BYTE)((BlockAddress[BlockCount] >> 8) & 0x00FF);
	tx_msg->PackedMsg.MsgData[4] = (BYTE)((BlockAddress[BlockCount] >> 16) & 0x00FF);
	tx_msg->PackedMsg.MsgData[5] = (BYTE)((BlockAddress[BlockCount] >> 24) & 0x00FF);

	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_WAITING;

return ucErrCode;
}

/**********************************************************************
BlockHeadRecvFcb-----下传BLOCK Head应答

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockHeadRecvFcb(VOID)
{
	UINT16 u16RetrunStatus = 0;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_HEAD_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);


			//允许传有效数据,准备发数据信息
			if (BLOCK_HEAD_OK == u16RetrunStatus){

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}

			else {
				
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG + 1;
			}
		}
	}
	MsgErrorProcess(STATUS_BLOCK_HEAD_OK, FALSE);
	return 0;
}

/**********************************************************************
BlockDataXmitFcb-----下传BLOCK数据

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockDataXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	BYTE *msg_data_ptr = (BYTE*)(BlockData[BlockCount]);
	UINT16 msg_num = (EveryBlockDataNum[BlockCount] << 1) / 6;
	for (UINT16 i = 0; i < msg_num + 2; ++i) {

		TX_MESSAGE_FUNCTION(i, BLOCK_DATA_SRVCODE, 8);
		if (i < msg_num) {

			tx_msg[i].PackedMsg.DataLen = 8;
		}
		else if (i == msg_num) {

			tx_msg[i].PackedMsg.DataLen = (EveryBlockDataNum[BlockCount] << 1) % 6 + 2;
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


	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_WAITING;
	
	return ucErrCode;
}

/**********************************************************************
BlockDataRecvFcb-----下传BLOCK数据应答

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockDataRecvFcb(VOID){
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;

	for (int i = 0; i < msg_num; ++i) {
		if (MESSAGE_FILLTER(BLOCK_DATA_SRVCODE)){
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			// dsp接收到的帧数与下发帧数相同, 则传CHECKSUM
			if (((EveryBlockDataNum[BlockCount] << 1) / 6 + 1) <= u16RetrunStatus){
	
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else {

				//FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt = MAX_ERROR_MSG + 1;
			}
		}


	}
	MsgErrorProcess(STATUS_BLOCK_DATATRANS_END, FALSE);
	return ucErrCode;
}



/**********************************************************************
BlockChecksumXmitFcb-----下传BLOCK CheckSum数据

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockChecksumXmitFcb(VOID)
{

	TX_MESSAGE_FUNCTION(0, BLOCK_CHECKSUM_SRVCODE, 6)
	tx_msg->PackedMsg.MsgData[0] = (BYTE)(BlockCheckSum[BlockCount] & 0x00FF);
	tx_msg->PackedMsg.MsgData[1] = (BYTE)((BlockCheckSum[BlockCount] >> 8) & 0x00FF);
	tx_msg->PackedMsg.MsgData[2] = (BYTE)((BlockCheckSum[BlockCount] >> 16) & 0x00FF);
	tx_msg->PackedMsg.MsgData[3] = (BYTE)((BlockCheckSum[BlockCount] >> 24) & 0x00FF);

	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_WAITING;


	return 0;
}

/**********************************************************************
BlockChecksumRecvFcb-----下传BLOCK数据校验和应答

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockChecksumRecvFcb(VOID)
{
	UINT16 u16RetrunStatus = 0;
	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_CHECKSUM_SRVCODE))
		{
			
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			if (CHECK_SUM_SUCCESFUL == u16RetrunStatus){

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
			else {

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt =
					MAX_ERROR_MSG + 1;
			}
		}
	}
	MsgErrorProcess(STATUS_BLOCK_CHECKSUM_OK, FALSE);
	
	return u16RetrunStatus;
}


/**********************************************************************
BlockProgStatusXmitFcb-----获取FLASH编程状态信息

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockProgStatusXmitFcb(VOID){

	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	TX_MESSAGE_FUNCTION(0, BLOCK_PROMG_STATUS_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_PROGRAM_WAITING;
	return ucErrCode;
}

/**********************************************************************
BlockProgStatusRecvFcb-----DSP上传编程状态

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockProgStatusRecvFcb(VOID)
{
	UINT16 u16RetrunStatus = 0;
	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	
	CheckRxMessageNum();
VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

for (int i = 0; i < msg_num; ++i) {

	if (MESSAGE_FILLTER(BLOCK_PROMG_STATUS_SRVCODE))
	{
		u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

		if (BLOCK_RECV_DATA_OK == u16RetrunStatus) {

			FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
		}
		else{

			FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].ereor_cnt =
				MAX_ERROR_MSG + 1;
		}
	}
}
MsgErrorProcess(STATUS_BLOCK_PROGRAM_WAITING, TRUE);
if ((MsgReceivedDoneFlagSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

	MsgReceivedDoneFlagSave = 0;
	//下发FLASH校验命令

	TX_MESSAGE_FUNCTION(0, VERIFY_SRVCODE, 2);
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;
}
return u16RetrunStatus;
}

DWORD CAN_FlashupdateMsgHandle::FlashUpdateCompleteRecv(VOID) {

	UINT16 u16RetrunStatus = 0;
	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);

	CheckRxMessageNum();
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if (MESSAGE_FILLTER(BLOCK_HEAD_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			if (FILE_TRANS_END == u16RetrunStatus) {


			}
		}
	}

	return u16RetrunStatus;
}
UINT32 CAN_FlashupdateMsgHandle::BlockMessageProcess_Packaged(void) {

	return 0;
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

			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;
		}
		// this state finish, refresh receive done state
		for (UINT16 i = NodeOffset; i < max_node_num; ++i) {

			FlashUpdateErrorMsg[i].receive_done = FALSE;
			
		}
	}
}



int CAN_FlashupdateMsgHandle::ParameterRefresh()
{
	MsgReceivedDoneFlagSave = 0;
	MsgErrorSave = 0;
	NodeSelect = 0;
	
	m_ucMsgClass = CAN_RESERVED_CLASS;
	m_pHostModuleItc = new _HOST_MODULE_ITC_T;
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;

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
		FlashUpdateErrorMsg[i].error_state_saved = STATUS_FLASH_UPDATE_INVALID;
		FlashUpdateErrorMsg[i].receive_done = FALSE;
	}
	BlockCount = 0;
	
	return 0;
}
