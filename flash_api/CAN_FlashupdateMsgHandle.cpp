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
	UINT16 i;

	m_ucMsgClass = CAN_RESERVED_CLASS;

	m_u16UpdatingModuleNo = 0;

	//初始化升级目标变量
	for (i = 0; i<11; i++)
	{
		m_tFlashupdateTarged[i].ucTargeAddr = 0xff;
		m_tFlashupdateTarged[i].ucTargetEnable = TARGET_UPDATE_DISABLE;
		m_tFlashupdateTarged[i].ucTaskHandled = TASK_HANDLED;
	}

	m_u16RespondModuleFlag = 0;


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
	tx_msg = new CAN_PACKED_PROTOCOL_U[500];
	rx_msg = new CAN_PACKED_PROTOCOL_U[200000];

	for (UINT16 i = 0; i < 500; ++i) {

		tx_msg[i].Frame = msg_init;
		for (UINT j = 0; j < 400; ++j) {

			rx_msg[i].Frame = msg_init;
		}
		
	}
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
GetMsgClass-----获取msg class


Parameters:
Return Value:
Precondition:
Postcondition:
**********************************************************************/
UCHAR CAN_FlashupdateMsgHandle::GetMsgClass(VOID)
{
	return m_ucMsgClass;
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


		m_pHostModuleItc->u16FlashupdateStatus = STATUS_SELCET_NODE;
		m_u16UpdaingNodeAdd = 0x10;
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
		//等待芯片解密信号
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

		break;

		//校验中
	case STATUS_FLASH_VERIFYING:

		VerifyRecvFcb();
		break;
		//校验完毕
	case STATUS_FLASH_UPDATE_OVER:


		//状态机复位
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;
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

	tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
	tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
	tx_msg->PackedMsg.b7ServiceCode = HANDS_COMM_SRVCODE;
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 4;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;


	for (int i = 0; i < msg_num; ++i) {

		
		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) && 
			(rx_msg[i].PackedMsg.b7ServiceCode == HANDS_COMM_SRVCODE))
		{

			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
			//握手成功
			if (HAND_OK_RESPOND == u16RetrunStatus)
			{

				//解密命令

				tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
				tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

				tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
				tx_msg->PackedMsg.b7ServiceCode = CHIP_DECODE_SRVCODE;
				tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
				tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
				tx_msg->PackedMsg.b1RsRq = RS_MSG;
				tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
				tx_msg->PackedMsg.DataLen = 2;

				VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
				//等待解密信息反馈,启动等待延时TBD
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_CHIP_DECODE;

			}

		}
	}
	return 0;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;

	for (int i = 0; i < msg_num; ++i) {
		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == CHIP_DECODE_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//芯片解密OK
			if (CHIP_DECODE_SUCCESS == u16RetrunStatus)
			{

				//下发获取API版本信息

				tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
				tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

				tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
				tx_msg->PackedMsg.b7ServiceCode = API_VERSION_SRVCODE;
				tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
				tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
				tx_msg->PackedMsg.b1RsRq = RS_MSG;
				tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
				tx_msg->PackedMsg.DataLen = 2;

				VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
				//等待解密信息反馈,启动等待延时TBD
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_API_VERSION;


			}
		}


	}
	return 0;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) && 
			(rx_msg[i].PackedMsg.b7ServiceCode == API_VERSION_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//API版本核对成功
			if (API_VESION_OK == u16RetrunStatus)
			{
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_API_OK;

			}
			else {


			}

		}
	}
	return 0;
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
	tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
	tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
	tx_msg->PackedMsg.b7ServiceCode = ERASE_SECTOR_SRVCODE;
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 4;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) && 
			(rx_msg[i].PackedMsg.b7ServiceCode == ERASE_SECTOR_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
			if (ERASE_SUCCESFULL == u16RetrunStatus)
			{
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_ERASED;

			}

		}

	}
	return 0;
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
	tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
	tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
	tx_msg->PackedMsg.b7ServiceCode = PROGRAM_SRVCODE;
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 2;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) && 
			(rx_msg[i].PackedMsg.b7ServiceCode == PROGRAM_SRVCODE))
		{

			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);


			if (PROGRAM_ENABLE == u16RetrunStatus)
			{
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_PROGRAM_ENABLE;

			}

			//擦除失败TBD
			else if(PROGRAM_DIABLE == u16RetrunStatus)
			{

			}

		}
	}
	return 0;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == VERIFY_SRVCODE))
		{
			FlashUpdateProgress[rx_msg[i].PackedMsg.b6SourceMacId] += 2;
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//校验成功,升级成功,反馈给后台TBD
			if (VERIFY_OK == u16RetrunStatus)
			{
				//当前BLOCK号加1
				BlockCount++;
				if (BlockCount >= BlockAmount)
				{
					tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
					tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

					tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
					tx_msg->PackedMsg.b7ServiceCode = BLOCK_HEAD_SRVCODE;
					tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
					tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
					tx_msg->PackedMsg.b1RsRq = RS_MSG;
					tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
					tx_msg->PackedMsg.DataLen = 8;

					// send 0x00 restart dsp
					tx_msg->PackedMsg.MsgData[0] = 0x00;
					tx_msg->PackedMsg.MsgData[1] = 0x00;

					tx_msg->PackedMsg.MsgData[2] = 0x00;
					tx_msg->PackedMsg.MsgData[3] = 0x00;
					tx_msg->PackedMsg.MsgData[4] = 0x00;
					tx_msg->PackedMsg.MsgData[5] = 0x00;
					m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;
					FlashUpdateProgress[rx_msg[i].PackedMsg.b6SourceMacId] = 100;

				}

				//还有BLOCK需传输,
				else
				{
					
					//状态机回到传输下一个BLOCK头
					m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
				}

			}

			//校验失败,需要重新擦除FLASH
			else
			{
				//--20100408:直接退出--
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
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


	tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
	tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
	tx_msg->PackedMsg.b7ServiceCode = BLOCK_HEAD_SRVCODE;
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 8;

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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == BLOCK_HEAD_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);


			//允许传有效数据,准备发数据信息
			if (BLOCK_HEAD_OK == u16RetrunStatus)
			{

				m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_OK;


			}

			//文件传输结束
			else if (FILE_TRANS_END == u16RetrunStatus)
			{
				
					m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
		}
	}
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

		tx_msg[i].PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
		tx_msg[i].PackedMsg.b7ServiceCode = BLOCK_DATA_SRVCODE;
		tx_msg[i].PackedMsg.b10MsgClass = m_ucMsgClass;
		tx_msg[i].PackedMsg.b1Fragment = NONFRAG_MSG;
		tx_msg[i].PackedMsg.b1RsRq = RS_MSG;
		tx_msg[i].PackedMsg.b6SourceMacId = MAC_ID_MON;
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


	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_END;
	
	return ucErrCode;
}

/**********************************************************************
BlockDataRecvFcb-----下传BLOCK数据应答

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockDataRecvFcb(VOID)
{

	
	return 0;
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
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;

	for (int i = 0; i < msg_num; ++i) {
		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == BLOCK_DATA_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			// dsp接收到的帧数与下发帧数相同, 则传CHECKSUM
			//if (((EveryBlockDataNum[BlockCount] << 1) / 6 + 2) == u16RetrunStatus)
			{
				tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
				tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

				tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
				tx_msg->PackedMsg.b7ServiceCode = BLOCK_CHECKSUM_SRVCODE;
				tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
				tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
				tx_msg->PackedMsg.b1RsRq = RS_MSG;
				tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
				tx_msg->PackedMsg.DataLen = 6;
				tx_msg->PackedMsg.MsgData[0] = (BYTE)(BlockCheckSum[BlockCount] & 0x00FF);
				tx_msg->PackedMsg.MsgData[1] = (BYTE)((BlockCheckSum[BlockCount] >> 8) & 0x00FF);
				tx_msg->PackedMsg.MsgData[2] = (BYTE)((BlockCheckSum[BlockCount] >> 16) & 0x00FF);
				tx_msg->PackedMsg.MsgData[3] = (BYTE)((BlockCheckSum[BlockCount] >> 24) & 0x00FF);

				VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

				m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_WAITING;

			}
		}


	}
	return ucErrCode;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == BLOCK_CHECKSUM_SRVCODE))
		{
			
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			if (CHECK_SUM_SUCCESFUL == u16RetrunStatus){

				m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_OK;
			}
			else {

				//KERNEL_DATA_CHKSUM_ERROR
			}
		}
	}
	return u16RetrunStatus;
}


/**********************************************************************
BlockProgStatusXmitFcb-----获取FLASH编程状态信息

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockProgStatusXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
	tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
	tx_msg->PackedMsg.b7ServiceCode = BLOCK_PROMG_STATUS_SRVCODE;
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 2;



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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == BLOCK_PROMG_STATUS_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			if (BLOCK_RECV_DATA_OK == u16RetrunStatus)
			{
				//下发FLASH校验命令
				tx_msg->PackedMsg.RemoteFlag = 0;// 数据帧
				tx_msg->PackedMsg.ExternFlag = 0;// 标准帧

				tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
				tx_msg->PackedMsg.b7ServiceCode = VERIFY_SRVCODE;
				tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
				tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
				tx_msg->PackedMsg.b1RsRq = RS_MSG;
				tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
				tx_msg->PackedMsg.DataLen = 2;

				VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;

				return 0;
			}
			//本次BLOCK编程失败,需重发TBD
			else
			{
				//--20100408:直接退出--
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
		}
	}
	return 0;
}

/**********************************************************************
FlashupdateNodeGet-----获取要升级的节点

Parameters:

Return Value: =0表示升级完成,=1还有节点待升级
Precondition:
Postcondition:
**********************************************************************/
UCHAR CAN_FlashupdateMsgHandle::FlashupdateNodeGet(VOID)
{

	return 0;

}

/**********************************************************************
FlashupdateTaskReset-----FLASH UPDATE 某项任务启动

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::FlashupdateTaskReset(VOID)
{
	UCHAR i;

	for (i = 0; i<11; i++)
	{
		if (TARGET_UPDATE_ENABLE == m_tFlashupdateTarged[i].ucTargetEnable)
		{
			m_tFlashupdateTarged[i].ucTaskHandled = TASK_HANDLE_NON;
		}

	}

}


/**********************************************************************
FlashupdateTaskHandle-----FLASH UPDATE 某项任务是否都已处理完毕

Parameters:

Return Value: =0表示处理完成,=1还有节点待处理
Precondition:
Postcondition:
**********************************************************************/
UCHAR CAN_FlashupdateMsgHandle::FlashupdateTaskHandle(UCHAR ucRecvAddr)
{
	UCHAR i, ucRet;
	ucRet = 0;

	for (i = 0; i<11; i++)
	{
		//升级使能
		if (TARGET_UPDATE_ENABLE == m_tFlashupdateTarged[i].ucTargetEnable)
		{
			if (ucRecvAddr == m_tFlashupdateTarged[i].ucTargeAddr)
			{
				m_tFlashupdateTarged[i].ucTaskHandled = TASK_HANDLED;

				if (0 == i)
				{
					ucRet = 0;
					break;
				}
			}

			else if (TASK_HANDLE_NON == m_tFlashupdateTarged[i].ucTaskHandled)
			{
				ucRet = 1;
			}

		}

	}

	return ucRet;

}



/**********************************************************************
ResetFlsUpdateMoudele-----复位各个模块升级为未使能
主要考虑在处理完握手应答信号之后使用

Parameters:

Return Value: N/A
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::ResetFlsUpdateMoudele(VOID)
{
	//复位升级目标变量
	for (UINT16 i = 0; i<11; i++)
	{
		m_tFlashupdateTarged[i].ucTargeAddr = 0xff;
		m_tFlashupdateTarged[i].ucTargetEnable = TARGET_UPDATE_DISABLE;
		m_tFlashupdateTarged[i].ucTaskHandled = TASK_HANDLED;
	}
}



UINT32 CAN_FlashupdateMsgHandle::BlockMessageProcess_Packaged(void) {

	return 0;
}

