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
GetMsgClass-----��ȡmsg class


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
FlashUpdateRoutine-----FLAHS UPDATE������


Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::FlashUpdateRoutine(VOID)
{

	switch (m_pHostModuleItc->u16FlashupdateStatus)
	{
		//��ʼ����
		//��ȡ��������
	case STATUS_FLASH_START:

		ParameterRefresh();
		NodeSelect = (m_u16UpdaingNodeAdd == 0x3F) ?
					 (UINT64)0xFFFF : ((UINT64)0x0001L << (Module_number - 1));
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_SELCET_NODE;
		//m_u16UpdaingNodeAdd = 0x10;
		m_ucMsgClass = CAN_RESERVED_CLASS;
		m_pHostModuleItc->u16FlashUpdateKernelFlag = 0x95;
		break;

		//��ָ���ڵ㿪ʼ����
		//���ȷ���������
	case STATUS_SELCET_NODE:

		HandCommXmitFcb();
		break;

		//�ȴ�����Ӧ���ź�
	case STATUS_WAITING_HANDS_RESPOND:
		//DO NOTHING
		HandCommXmitFcb();
		HandCommRecvChipDecodeXmit();
		break;
		//�ȴ�оƬ�����ź�, ��ҪAPI�汾��Ϣ
	case STATUS_WAITING_CHIP_DECODE:
		ChipDecodeRecvFcb();
		break;
		//�ȴ�API�汾ȷ����Ϣ
	case STATUS_WAITING_API_VERSION:
		
		ApiVersionRecvFcb();
		break;
		//API VERSION OK
	case STATUS_API_OK:
		//�·���������
		EraseSectorXmitFcb();

		break;
		//�ȴ���������Ӧ��
	case STATUS_FLASH_ERASE_GOING:
		EraseSectorRecvFcb();
		break;
		//�����ɹ�
	case STATUS_FLASH_ERASED:
		//�·������������
		ProgramXmitFcb();

		break;
		//�ȴ��������״̬
	case STATUS_PROGRAM_PERMIT_WAITING:
		ProgramRecvFcb();
		break;
		//�������
	case STATUS_PROGRAM_ENABLE:		
		//�·�BLOCKͷ
		BlockHeadXmitFcb();
		break;
		//ͷBLOCK������
	case STATUS_BLOCK_HEAD_WAITING:
		BlockHeadRecvFcb();
		break;
		//��һ��BLOCKͷ
	case STATUS_NEXT_BLOCK_HEAD:
		//�·�BLOCKͷ
		BlockHeadXmitFcb();
		break;

		//ͷ�������,��ʼ��������
	case STATUS_BLOCK_HEAD_OK:

		BlockDataXmitFcb();

		//BLOCK���ݴ��������
	case STATUS_BLOCK_DATATRANS_WAITING:

		BlockDataRecvFcb();
		break;

		//BLOCK���ݴ������, �·�У������
	case STATUS_BLOCK_DATATRANS_END:

		BlockChecksumXmitFcb();
		break;
		//blockУ����
	case STATUS_BLOCK_CHECKSUM_WAITING:

		BlockChecksumRecvFcb();
		break;
		//BLOCKУ��OK, �·��������
	case STATUS_BLOCK_CHECKSUM_OK:

		BlockProgStatusXmitFcb();
		break;
		//���״̬
	case STATUS_BLOCK_PROGRAM_WAITING:
		
		BlockProgStatusRecvFcb();
		break;

		//������
	case STATUS_BLOCK_PROGRAM_COMPLETE:

		break;
		//�ļ��������
	case STATUS_FILE_TRANS_END:

		FlashUpdateCompleteRecv();
		break;

		//У����
	case STATUS_FLASH_VERIFYING:

		VerifyRecvFcb();
		break;
		//У�����
	case STATUS_FLASH_UPDATE_OVER:


		//״̬����λ
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;
		break;
	default:


		//״̬����λ
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;

		break;

	}


}


/**********************************************************************
HandCommXmitFcb-----�����ź��·�

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::HandCommXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	//����������

	tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
	tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

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

	//�ȴ�����Ӧ���ź�
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_HANDS_RESPOND;

	return ucErrCode;
}

/**********************************************************************
HandCommRecvFcb-----�����źŽ���

Parameters:		=0xaa��ʾ���ֳɹ�

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
			//���ֳɹ�
			if (HAND_OK_RESPOND == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
				
			}

		}
	}
	//�ȴ�������Ϣ����,�����ȴ���ʱTBD
	MsgErrorProcess(STATUS_WAITING_CHIP_DECODE, TRUE);
	//��������
	if ((MsgErrorSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		MsgErrorSave = 0;
		tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
		tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

		tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
		tx_msg->PackedMsg.b7ServiceCode = CHIP_DECODE_SRVCODE;
		tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
		tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
		tx_msg->PackedMsg.b1RsRq = RS_MSG;
		tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
		tx_msg->PackedMsg.DataLen = 2;

		VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	}

	return u16RetrunStatus;
}


/**********************************************************************
ChipDecodeXmitFcb-----���������·�--����

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
ChipDecodeRecvFcb-----�����źŽ���

Parameters:		=0xaa��ʾ���ֳɹ�

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

			//оƬ����OK
			if (CHIP_DECODE_SUCCESS == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
				
			}
		}
	}
	//�ȴ�������Ϣ����,�����ȴ���ʱTBD
	MsgErrorProcess(STATUS_WAITING_API_VERSION, TRUE);
	//�·���ȡAPI�汾��Ϣ
	if ((MsgErrorSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		MsgErrorSave = 0;
		tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
		tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

		tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
		tx_msg->PackedMsg.b7ServiceCode = API_VERSION_SRVCODE;
		tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
		tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
		tx_msg->PackedMsg.b1RsRq = RS_MSG;
		tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
		tx_msg->PackedMsg.DataLen = 2;

		VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);

	}
	return u16RetrunStatus;
}


/**********************************************************************
ApiVersionXmitFcb-----API�汾�˶������·�--����

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
ApiVersionRecvFcb-----API�汾�˶��źŽ���

Parameters:		=0x6C��ʾ�汾ƥ��,�������
=14 �汾��ƥ��

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

			//API�汾�˶Գɹ�
			if (API_VESION_OK == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;

			}
			else {


			}

		}
	}
	MsgErrorProcess(STATUS_API_OK, FALSE);
	return u16RetrunStatus;
}


/**********************************************************************
EraseSectorXmitFcb-----�������������·�

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::EraseSectorXmitFcb(VOID)
{
	tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
	tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
	tx_msg->PackedMsg.b7ServiceCode = ERASE_SECTOR_SRVCODE;
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 4;
	*((UINT16*)(tx_msg->PackedMsg.MsgData)) = ERASE_SECTOR_ALL;
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
	//�·���������,�ȴ�������Ϣ����
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_ERASE_GOING;

	return 0;
}

/**********************************************************************
EraseSectorRecvFcb-----�������������

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

		}

	}
	MsgErrorProcess(STATUS_FLASH_ERASED, FALSE);
	return u16RetrunStatus;
}

/**********************************************************************
ProgramXmitFcb-----��������·�

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ProgramXmitFcb(VOID)
{
	tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
	tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

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
ProgramRecvFcb-----������������

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

			//����ʧ��TBD
			else if(PROGRAM_DIABLE == u16RetrunStatus)
			{

			}

		}
	}
	MsgErrorProcess(STATUS_PROGRAM_ENABLE, FALSE);
	return u16RetrunStatus;
}

/**********************************************************************
VerifyXmitFcb-----У�������·�

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
VerifyRecvFcb-----У�������

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

			//У��ɹ�,�����ɹ�,��������̨TBD
			if (VERIFY_OK == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
				
			}

			//У��ʧ��,��Ҫ���²���FLASH
			else
			{
				//--20100408:ֱ���˳�--
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
		}
	}
	MsgErrorProcess(STATUS_FLASH_VERIFYING, TRUE);
	if ((MsgErrorSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		MsgErrorSave = 0;
		//��ǰBLOCK�ż�1
		BlockCount++;
		if (BlockCount >= BlockAmount)
		{
			tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
			tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

			tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
			tx_msg->PackedMsg.b7ServiceCode = BLOCK_HEAD_SRVCODE;
			tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
			tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
			tx_msg->PackedMsg.b1RsRq = RQ_MSG;
			tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
			tx_msg->PackedMsg.DataLen = 8;

			// send 0x00 restart dsp
			tx_msg->PackedMsg.MsgData[0] = 0x00;
			tx_msg->PackedMsg.MsgData[1] = 0x00;

			tx_msg->PackedMsg.MsgData[2] = 0x00;
			tx_msg->PackedMsg.MsgData[3] = 0x00;
			tx_msg->PackedMsg.MsgData[4] = 0x00;
			tx_msg->PackedMsg.MsgData[5] = 0x00;
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FILE_TRANS_END;

		}

		//����BLOCK�贫��,
		else
		{

			//״̬���ص�������һ��BLOCKͷ
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
		}
	}
	return 0;
}


/**********************************************************************
BlockHeadXmitFcb-----�´�BLOCK��С����ַ

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockHeadXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;


	tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
	tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

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
BlockHeadRecvFcb-----�´�BLOCK HeadӦ��

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


			//������Ч����,׼����������Ϣ
			if (BLOCK_HEAD_OK == u16RetrunStatus)
			{
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}

			else 
			{
				
				//m_pHostModuleItc->u16FlashupdateStatus = STATUS_FILE_TRANS_END;
			}
		}
	}
	MsgErrorProcess(STATUS_BLOCK_HEAD_OK, FALSE);
	return 0;
}

/**********************************************************************
BlockDataXmitFcb-----�´�BLOCK����

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


	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_WAITING;
	
	return ucErrCode;
}

/**********************************************************************
BlockDataRecvFcb-----�´�BLOCK����Ӧ��

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

			// dsp���յ���֡�����·�֡����ͬ, ��CHECKSUM
			if (((EveryBlockDataNum[BlockCount] << 1) / 6 + 2) == u16RetrunStatus){
	
				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;
			}
		}


	}
	MsgErrorProcess(STATUS_BLOCK_DATATRANS_END, FALSE);
	return ucErrCode;
}



/**********************************************************************
BlockChecksumXmitFcb-----�´�BLOCK CheckSum����

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockChecksumXmitFcb(VOID)
{

	tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
	tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

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


	return 0;
}

/**********************************************************************
BlockChecksumRecvFcb-----�´�BLOCK����У���Ӧ��

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

				//KERNEL_DATA_CHKSUM_ERROR
			}
		}
	}
	MsgErrorProcess(STATUS_BLOCK_CHECKSUM_OK, FALSE);
	
	return u16RetrunStatus;
}


/**********************************************************************
BlockProgStatusXmitFcb-----��ȡFLASH���״̬��Ϣ

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockProgStatusXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
	tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

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
BlockProgStatusRecvFcb-----DSP�ϴ����״̬

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

			if (BLOCK_RECV_DATA_OK == u16RetrunStatus){

				FlashUpdateErrorMsg[rx_msg[i].PackedMsg.b6SourceMacId].receive_done = TRUE;				
			}
			//����BLOCK���ʧ��,���ط�TBD
			else
			{
				//--20100408:ֱ���˳�--
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
		}
	}
	MsgErrorProcess(STATUS_BLOCK_PROGRAM_WAITING, TRUE);
	if ((MsgErrorSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {
		MsgErrorSave = 0;
		//�·�FLASHУ������
		tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
		tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

		tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
		tx_msg->PackedMsg.b7ServiceCode = VERIFY_SRVCODE;
		tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
		tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
		tx_msg->PackedMsg.b1RsRq = RS_MSG;
		tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
		tx_msg->PackedMsg.DataLen = 2;

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

			if (FlashUpdateErrorMsg[i].ereor_cnt != 123) {

				FlashUpdateErrorMsg[i].ereor_cnt++;

			}
		}

		if ((FlashUpdateErrorMsg[i].receive_done) || (FlashUpdateErrorMsg[i].ereor_cnt > MAX_ERROR_MSG)) {

			MsgErrorSave |= ((UINT64)1) << i;	
			if ((FlashUpdateErrorMsg[i].ereor_cnt > MAX_ERROR_MSG)&&(FlashUpdateErrorMsg[i].ereor_cnt < MAX_ERROR_MSG+2)){

				FlashUpdateErrorMsg[i].ereor_cnt = 123;
				FlashUpdateErrorMsg[i].error_state_saved = flash_update_state;
			}
		}
	}
	if ((MsgErrorSave &(NodeSelect << NodeOffset)) == (NodeSelect << NodeOffset)) {

		m_pHostModuleItc->u16FlashupdateStatus = flash_update_state;
		MsgErrorSave = (IsNot == FALSE) ? 0 : MsgErrorSave;
		// this state gameover, refresh receive done state
		for (UINT16 i = NodeOffset; i < max_node_num; ++i) {

			FlashUpdateErrorMsg[i].receive_done = FALSE;
			
		}
	}
}



int CAN_FlashupdateMsgHandle::ParameterRefresh()
{
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
	return 0;
}
