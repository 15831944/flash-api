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

	//��ʼ������Ŀ�����
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


		m_pHostModuleItc->u16FlashupdateStatus = STATUS_SELCET_NODE;
		m_u16UpdaingNodeAdd = 0x10;
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
		//�ȴ�оƬ�����ź�
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;


	for (int i = 0; i < msg_num; ++i) {

		
		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) && 
			(rx_msg[i].PackedMsg.b7ServiceCode == HANDS_COMM_SRVCODE))
		{

			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
			//���ֳɹ�
			if (HAND_OK_RESPOND == u16RetrunStatus)
			{

				//��������

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
				//�ȴ�������Ϣ����,�����ȴ���ʱTBD
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_CHIP_DECODE;

			}

		}
	}
	return 0;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus = 0;

	for (int i = 0; i < msg_num; ++i) {
		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == CHIP_DECODE_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//оƬ����OK
			if (CHIP_DECODE_SUCCESS == u16RetrunStatus)
			{

				//�·���ȡAPI�汾��Ϣ

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
				//�ȴ�������Ϣ����,�����ȴ���ʱTBD
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_API_VERSION;


			}
		}


	}
	return 0;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) && 
			(rx_msg[i].PackedMsg.b7ServiceCode == API_VERSION_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//API�汾�˶Գɹ�
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

			//����ʧ��TBD
			else if(PROGRAM_DIABLE == u16RetrunStatus)
			{

			}

		}
	}
	return 0;
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == VERIFY_SRVCODE))
		{
			FlashUpdateProgress[rx_msg[i].PackedMsg.b6SourceMacId] += 2;
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//У��ɹ�,�����ɹ�,��������̨TBD
			if (VERIFY_OK == u16RetrunStatus)
			{
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

				//����BLOCK�贫��,
				else
				{
					
					//״̬���ص�������һ��BLOCKͷ
					m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
				}

			}

			//У��ʧ��,��Ҫ���²���FLASH
			else
			{
				//--20100408:ֱ���˳�--
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
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
	if (msg_num == 0)return 0;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON) &&
			(rx_msg[i].PackedMsg.b7ServiceCode == BLOCK_HEAD_SRVCODE))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);


			//������Ч����,׼����������Ϣ
			if (BLOCK_HEAD_OK == u16RetrunStatus)
			{

				m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_OK;


			}

			//�ļ��������
			else if (FILE_TRANS_END == u16RetrunStatus)
			{
				
					m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
		}
	}
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


	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_END;
	
	return ucErrCode;
}

/**********************************************************************
BlockDataRecvFcb-----�´�BLOCK����Ӧ��

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
BlockChecksumXmitFcb-----�´�BLOCK CheckSum����

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

			// dsp���յ���֡�����·�֡����ͬ, ��CHECKSUM
			//if (((EveryBlockDataNum[BlockCount] << 1) / 6 + 2) == u16RetrunStatus)
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

			}
		}


	}
	return ucErrCode;
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

				return 0;
			}
			//����BLOCK���ʧ��,���ط�TBD
			else
			{
				//--20100408:ֱ���˳�--
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			}
		}
	}
	return 0;
}

/**********************************************************************
FlashupdateNodeGet-----��ȡҪ�����Ľڵ�

Parameters:

Return Value: =0��ʾ�������,=1���нڵ������
Precondition:
Postcondition:
**********************************************************************/
UCHAR CAN_FlashupdateMsgHandle::FlashupdateNodeGet(VOID)
{

	return 0;

}

/**********************************************************************
FlashupdateTaskReset-----FLASH UPDATE ĳ����������

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
FlashupdateTaskHandle-----FLASH UPDATE ĳ�������Ƿ��Ѵ������

Parameters:

Return Value: =0��ʾ�������,=1���нڵ������
Precondition:
Postcondition:
**********************************************************************/
UCHAR CAN_FlashupdateMsgHandle::FlashupdateTaskHandle(UCHAR ucRecvAddr)
{
	UCHAR i, ucRet;
	ucRet = 0;

	for (i = 0; i<11; i++)
	{
		//����ʹ��
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
ResetFlsUpdateMoudele-----��λ����ģ������Ϊδʹ��
��Ҫ�����ڴ���������Ӧ���ź�֮��ʹ��

Parameters:

Return Value: N/A
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::ResetFlsUpdateMoudele(VOID)
{
	//��λ����Ŀ�����
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

