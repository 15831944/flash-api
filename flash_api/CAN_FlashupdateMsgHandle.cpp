#include "stdafx.h"
#include "CAN_FlashupdateMsgHandle.h"
#include "ControlCan.h"

//The third headfiles
#include <stdio.h>
#include <assert.h>


//-----------------------------------------------------------------
//Private Constants and Macro Definitions
//Private Enumerated and Structure Definitions

#define	OS_BASE_YEAR		1900
#define	OS_BASE_CENTARY	20

#define SET_EVENT 						0x5555		//��ǰ��¼����
#define CLEAR_EVENT 						0x1111		//��ʷ��¼����

#define SET_PARA_SUPPORT 1
#define SET_PARA_NOT_SUPPORT 0

#define COMM_FAIL_TIMEOUT_CNT 5


//���͡����մ���������
typedef INT32(CAN_FlashupdateMsgHandle::*_MSG_XMIT_FCB)(VOID);
typedef INT32(CAN_FlashupdateMsgHandle::*_MSG_RECV_FCB)(VOID);

typedef struct
{
	UCHAR ucServiceCode;
	_MSG_RECV_FCB pMsgXmitFcb;
	_MSG_RECV_FCB pMsgRecvFcb;
}_CAN_FLASHUPDATE_MSG_T;



//��ȡHEX�ļ�������־����
#define READ_HEX_END_FLAG_VALID		0x6789
#define READ_HEX_END_FLAG_RESET 		0




typedef struct
{
	UCHAR ucFlashObj;
	UINT16 u16FlashModule;
	UINT16 u16DestNodeAdd;
}_FLASH_OBJ_T;

typedef struct
{
	//ģ���.0--��·,1~10��Ӧģ��1~10
	UINT16 u16FlashModule;
	UINT16 u16FlashFlagMask;
}_FLASH_FLASH_MASK_T;

//ģ����������Ӧ������ر�־
_FLASH_FLASH_MASK_T utFlashFlagMaskObj[] =
{
//byp
{ 0,0x01 },
//module 1
{ 1,0x02 },
//module 2
{ 2,0x04 },
//module 3
{ 3,0x08 },
//module 4
{ 4,0x10 },
//module 5
{ 5,0x20 },
//module 6
{ 6,0x40 },
//module 7
{ 7,0x80 },
//module 8
{ 8,0x100 },
//module 9
{ 9,0x200 },
//module 10
{ 10,0x400 }
};

#define FLASHFLAG_MASK_CNT (sizeof(utFlashFlagMaskObj) / sizeof (_FLASH_FLASH_MASK_T))


_FLASH_OBJ_T ucFlashUpdateNodeObj[] =
{
	//BYP
	{ FLASHUPDATE_OBJECT_BYP,FLASHUPDATE_MODUL0,0x0F },

	//REC
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL1,0x20 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL2,0x21 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL3,0x22 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL4,0x23 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL5,0x24 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL6,0x25 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL7,0x26 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL8,0x27 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL9,0x28 },
{ FLASHUPDATE_OBJECT_REC,FLASHUPDATE_MODUL10,0x29 },

//INV
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL1,0x10 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL2,0x11 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL3,0x12 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL4,0x13 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL5,0x14 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL6,0x15 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL7,0x16 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL8,0x17 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL9,0x18 },
{ FLASHUPDATE_OBJECT_INV,FLASHUPDATE_MODUL10,0x19 }



};

#define FLASHUPDATE_NODE_CNT (sizeof(ucFlashUpdateNodeObj) / sizeof (_FLASH_OBJ_T))

union WORD2BYTE
{
	WORD word;
	BYTE bytes[2];
};

union UINT2BYTE
{
	UINT uint;
	BYTE bytes[4];
};

//һ��BLOCK��1024��������,DSP�࿪�ٵĻ��������Ϊ1024��
#define BLOCK_SIZE	1024

//һ��CAN֡��ഫ����Ч����Ϊ3(6�ֽ�)
#define FRAME_VALID_DATA_SIZE		3

//һ��������BLOCK(1024��)��341֡(3����Ч��)+1֡(1����Ч��)=1024��
#define FRAME_ALL_NUM_ONE_BLOCK	342
//һ��������BLOCK����֡�����һ֡���ֵĸ���
#define LAST_FRAME_WORD_NUM_ONE_BLOCK 1
//һ��������BLOCK����֡�з����һ֡���ֵĸ���
#define NON_LAST_FRAME_WORD_NUM_ONE_BLOCK 3

//BLOCK �ط�����
#define BLOCK_RESEND_CNT	3


//only for test 72bytes
UCHAR ucFlashUpdateTestData[] =
{
	0xAA, 0x08, 0x00, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00,
	0x00 , 0x00 , 0x3E , 0x00 , 0x4E , 0xA4 , 0x2B , 0x00
	, 0x3E , 0x00 , 0xAC , 0xAD , 0xFF , 0xFF , 0x13 , 0xA0 , 0x00 , 0x00 , 0x00 , 0x00 , 0xFE , 0xFF
	, 0x14 , 0xA0 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0xFF , 0xFF
	, 0x40 , 0xAA , 0x00 , 0x00 , 0x00 , 0x00 , 0xFE , 0xFF , 0x42 , 0xAA , 0x00 , 0x00
	, 0x00 , 0x00 , 0x00 , 0x00 , 0xFE , 0xFF , 0x44 , 0xAA , 0x00 , 0x00 , 0x00 , 0x00
};

/**********************************************************************
CAN_FlashupdateMsgHandle-----constructor function


Parameters:
Return Value:
Precondition:
Postcondition:
**********************************************************************/
CAN_FlashupdateMsgHandle::CAN_FlashupdateMsgHandle(void)
{
	UINT16 i;

	m_ucMsgClass = CAN_RESERVED_CLASS;

	m_XmitMsg.ucMsgClass = m_ucMsgClass;
	m_XmitMsg.ucFrag = NONFRAG_MSG;
	m_u16UpdatingModuleNo = 0;

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;



	//��ʼ������Ŀ�����
	for (i = 0; i<11; i++)
	{
		m_tFlashupdateTarged[i].ucTargeAddr = 0xff;
		m_tFlashupdateTarged[i].ucTargetEnable = TARGET_UPDATE_DISABLE;
		m_tFlashupdateTarged[i].ucTaskHandled = TASK_HANDLED;
	}

	m_u16RespondModuleFlag = 0;

	//�������ȱ�־
	m_u16ProgramPorcess = PROGRESS_IN_RESET;

	m_pHostModuleItc = new _HOST_MODULE_ITC_T;
	m_XmitMsg.pData = m_ucXmitMsgBuf;

	tx_msg = new CAN_PACKED_PROTOCOL_U[500];
	rx_msg = new CAN_PACKED_PROTOCOL_U[500];
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
	
	UCHAR ucStateFlag;

	ucStateFlag = 0;

	//for test
	//static UCHAR ucTestFlag=0;

	//for test
	//	FlashupdateNodeGet();
	//	return;

	//��FLASH UDATE ״̬������

	switch (m_pHostModuleItc->u16FlashupdateStatus)
	{
		//��ʼ����
		//��ȡ��������
	case STATUS_FLASH_START:

		//�������ȱ�־
		m_u16ProgramPorcess = PROGRESS_IN_START;

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
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_HEAD_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		break;

		//ͷ�������
	case STATUS_BLOCK_HEAD_OK:
		//BLOCK���ݴ��������
	case STATUS_BLOCK_DATATRANS_WAITING:
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_DATA_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		break;

		//BLOCK���ݴ������
	case STATUS_BLOCK_DATATRANS_END:
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_CHECKSUM_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		break;
		//blockУ����
	case STATUS_BLOCK_CHECKSUM_WAITING:
		//do nothing
		break;
		//BLOCKУ��OK
	case STATUS_BLOCK_CHECKSUM_OK:
		//��ȡ��Ӧ�ı��״̬����
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_PROMG_STATUS_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);

		break;
		//���״̬
	case STATUS_BLOCK_PROGRAM_WAITING:
		//DO NOTHING
		break;

		//������
	case STATUS_BLOCK_PROGRAM_COMPLETE:
		/*
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = VERIFY_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		*/
		break;
		//�ļ��������
	case STATUS_FILE_TRANS_END:
		/*
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = VERIFY_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		*/
		break;

		//У����
	case STATUS_FLASH_VERIFYING:
		//do nothing
		break;
		//У�����
	case STATUS_FLASH_UPDATE_OVER:


		//״̬����λ
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;

		//�������ȱ�־��λ
		m_u16ProgramPorcess = PROGRESS_IN_RESET;
		break;
	default:

		//�������ȱ�־��λ
		m_u16ProgramPorcess = PROGRESS_IN_RESET;
		ucStateFlag = 1;
		break;

	}

	if ((1 == ucStateFlag) && (m_u16RespondModuleFlag != 0))
	{
		m_u16RespondModuleFlag = 0;
	}
}

/**********************************************************************
Msg_Xmit-----��Ϣ���ʹ���


Parameters:
pCanXmitMsg:Ӧ�ò����ڷ�����Ϣ����
Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::Msg_Xmit(IN CAN_XMIT_QUEUE_MSG_T * pCanXmitMsg)
{
	INT32 i, ret;
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;


	//���µ�ǰ��������
	m_XmitQueueMsg = *pCanXmitMsg;

	//����RS/RE��־��������Э��㽻������Ϣ
	m_XmitMsg.ucServiceCode = m_XmitQueueMsg.ucServiceCode;
	m_XmitMsg.ucSourceId = MAC_ID_MON;
	m_XmitMsg.ucDestinationId = (m_XmitQueueMsg.u16DestinationId & 0x00ff);
	m_XmitMsg.u16Length = 0;

	//���Ҷ�Ӧ�Ĵ���������ִ��
	for (i = 0; i < CAN_FLASHUPDATE_MSG_OBJ_CNT; i++)
	{
		if (m_XmitQueueMsg.ucServiceCode == CanFlashupdateMsgObjs[i].ucServiceCode)
		{
			if (NULL != CanFlashupdateMsgObjs[i].pMsgXmitFcb)
			{
				(this->*(CanFlashupdateMsgObjs[i].pMsgXmitFcb))();
				break;
			}
		}
	}

	//Can not find the same service code, then get a error
	if (CAN_FLASHUPDATE_MSG_OBJ_CNT == i)
	{
		ret = CAN_MSG_HANDLE_INVALID_SRVC_COD;
		CAN_APP_TRACE("Service code [%d] is invalid\n", m_XmitQueueMsg.ucServiceCode);
	}

	else
	{
		//Xmit to protocol layer
		ret = m_pProtocolMgr->MsgXmit(&m_XmitMsg);
	}

	return ret;

}


/**********************************************************************
Msg_Recv-----��Ϣ���մ���

Parameters:
pCanRecvMsg:��Э����ϴ�����Ϣ����
Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::Msg_Recv(CAN_APDU_T *pCanRecvMsg)
{
	INT32 i;

	//���µ�ǰ��������
	m_RecvMsg = *pCanRecvMsg;

	//���Ҷ�Ӧ�Ĵ���������ִ��
	for (i = 0; i < CAN_FLASHUPDATE_MSG_OBJ_CNT; i++)
	{
		//���ӽ��յ�����---���������������ʱֻ�ܽ�����������֡
		if (m_RecvMsg.ucServiceCode == CanFlashupdateMsgObjs[i].ucServiceCode)
		{
			//REC
			if (FLASHUPDATE_OBJECT_REC == m_pHostModuleItc->sFlashUpdateFlag)
			{

				if ((0x20 <= m_RecvMsg.ucSourceId) && (0x29 >= m_RecvMsg.ucSourceId))
				{
					if (NULL != CanFlashupdateMsgObjs[i].pMsgRecvFcb)
					{
						(this->*(CanFlashupdateMsgObjs[i].pMsgRecvFcb))();
						break;
					}
				}
			}
			//INV
			else if (FLASHUPDATE_OBJECT_INV == m_pHostModuleItc->sFlashUpdateFlag)
			{

				if ((0x10 <= m_RecvMsg.ucSourceId) && (0x19 >= m_RecvMsg.ucSourceId))
				{
					if (NULL != CanFlashupdateMsgObjs[i].pMsgRecvFcb)
					{
						(this->*(CanFlashupdateMsgObjs[i].pMsgRecvFcb))();
						break;
					}
				}
			}
			//BYP
			else if (FLASHUPDATE_OBJECT_BYP == m_pHostModuleItc->sFlashUpdateFlag)
			{
				if (0x0f == m_RecvMsg.ucSourceId)
				{
					if (NULL != CanFlashupdateMsgObjs[i].pMsgRecvFcb)
					{
						(this->*(CanFlashupdateMsgObjs[i].pMsgRecvFcb))();
						break;
					}
				}
			}

			//do nothing
			else
			{

			}

		}
	}

	//Can not find the same service code, then get a error
	if (CAN_FLASHUPDATE_MSG_OBJ_CNT == i)
	{
		//CAN_APP_TRACE("Service code [%d] is invalid\n", m_RecvMsg.ucServiceCode);
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

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;// ��д��һ֡��ID
	tx_msg->PackedMsg.b7ServiceCode = HANDS_COMM_SRVCODE;// ��������
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 4;// ���ݳ���4���ֽ�
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
	if (msg_num == 0)return;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus;


	for (int i = 0; i < msg_num; ++i) {

		u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON))
		{

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
	if (msg_num == 0)return;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	UINT16 u16RetrunStatus;

	for (int i = 0; i < msg_num; ++i) {
		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//оƬ����OK
			if (CHIP_DECODE_SUCCESS == u16RetrunStatus)
			{

				//�·���ȡAPI�汾��Ϣ

				tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
				tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

				tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;// ��д��һ֡��ID
				tx_msg->PackedMsg.b7ServiceCode = API_VERSION_SRVCODE;// ��������
				tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
				tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
				tx_msg->PackedMsg.b1RsRq = RS_MSG;
				tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
				tx_msg->PackedMsg.DataLen = 2;// ���ݳ���4���ֽ�

				VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
				//�ȴ�������Ϣ����,�����ȴ���ʱTBD
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_API_VERSION;


			}
		}


	}

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
	UINT16 u16RetrunStatus;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	if (msg_num == 0)return;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);

			//API�汾�˶Գɹ�
			if (API_VESION_OK == u16RetrunStatus)
			{
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_API_OK;

			}


		}
	}

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
	UINT16 u16RetrunStatus;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	if (msg_num == 0)return;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON))
		{
			u16RetrunStatus = *(UINT16 *)(rx_msg[i].PackedMsg.MsgData);
			if (ERASE_SUCCESFULL == u16RetrunStatus)
			{
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_ERASED;

			}

		}

	}
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


}

/**********************************************************************
ProgramRecvFcb-----��������

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ProgramRecvFcb(VOID)
{
	UINT16 u16RetrunStatus;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	if (msg_num == 0)return;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON))
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


	//������Э��㽻������Ϣ
	m_XmitMsg.ucRsRq = RS_MSG;

	m_XmitMsg.u16Length = 0;
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;

	//�����ط���ʱ��
	WaitBlockFlashVerifyRespondTimerPost();

	//��ʱ����������
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[8])
	{
		m_u16TimerExpiredCnt[8] = RESEND_WAITING_START_CNT;
	}

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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	UINT16 j;

	//for test
	printf("Flash update: verify recv1,m_u16CurrentBlockNum=%d,m_u16BlockNumOfSection=%d\n", m_u16CurrentBlockNum, m_u16BlockNumOfSection);


	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;
	/*
	//����յ��ķ�������Ӧ����Ϣ�����˳�,��δ���TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/

	//������Ȳ��Ϸ�������˴ν��յ���Ӧ���ź�
	if (PROGRESS_IN_FLASH_PROG_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//У��ɹ�,�����ɹ�,��������̨TBD
	if (VERIFY_OK == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			//�������ȱ�־
			m_u16ProgramPorcess = PROGRESS_IN_PROG_ENA_OK;

			//����SECTION����BLOCK������
			if (m_u16CurrentBlockNum >= (m_u16BlockNumOfSection - 1))
			{
				//׼��ȥ����һ��section
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_PROGRAM_ENABLE;
				//for test
				printf("Flash update: verify recv11\n");

			}

			//����BLOCK�贫��,
			else
			{
				//��ǰBLOCK�ż�1
				m_u16CurrentBlockNum++;
				//BLOCK��ַ��1024
				m_uBlockAddress = m_uBlockAddress + 1024;
				//״̬���ص�������һ��BLOCKͷ
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
				//for test
				printf("Flash update: verify recv22\n");
			}

			//�رն�ʱ��
			WaitBlockFlashVerifyRespondTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[8] = RESEND_WAITING_RESET_CNT;

			//			m_u16ResendCnt = BLOCK_RESEND_CNT;
		}

		//wait othger node repond
		else
		{
			;//do nothing
		}

	}

	//У��ʧ��,��Ҫ���²���FLASH
	else
	{
		//״̬���ص�׼���·���������״̬
		//m_pHostModuleItc->u16FlashupdateStatus = STATUS_API_OK;
		//--20100408:ֱ���˳�--
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;
		//�������ȱ�־��λ
		m_u16ProgramPorcess = PROGRESS_IN_RESET;

		//���ظ���̨ʧ����Ϣ
		for (j = 0; j<11; j++)
		{
			m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
		}

		for (j = 0; j<TIMER_CNT_LEN; j++)
		{
			m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
		}
	}


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
	CHAR* BlcokHead;





	tx_msg->PackedMsg.RemoteFlag = 0;// ����֡
	tx_msg->PackedMsg.ExternFlag = 0;// ��׼֡

	tx_msg->PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;
	tx_msg->PackedMsg.b7ServiceCode = BLOCK_HEAD_SRVCODE;
	tx_msg->PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg->PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg->PackedMsg.b1RsRq = RS_MSG;
	tx_msg->PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg->PackedMsg.DataLen = 8;

	for (int i = 0; i < 6; ++i) {

		tx_msg->PackedMsg.MsgData[i] = BlcokHead[i];
	}
	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg->Frame, 1);
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_WAITING;

	return ucErrCode;
}

/**********************************************************************
BlockHeadRecvFcb-----�´�BLOCK����Ӧ��

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockHeadRecvFcb(VOID)
{
	UINT16 u16RetrunStatus;

	int msg_num = VCI_GetReceiveNum(device_type, device_ind, can_ind);
	if (msg_num == 0)return;
	VCI_Receive(device_type, device_ind, can_ind, &(rx_msg->Frame), msg_num, 1);

	for (int i = 0; i < msg_num; ++i) {

		if ((rx_msg[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&
			(rx_msg[i].PackedMsg.b6DestinationMacId == MAC_ID_MON))
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
				//���DSP���������ɹ���Ϣ,ͬʱ�����������־�˶�,
				//�����ر�־��ʾ�ļ���ȷ�������Ϊ���������ɹ�
				//��ر�����ΪHEX�ļ���û����,��ʧ��
				if (READ_HEX_END_FLAG_VALID != m_u16ReadHexFileEnd)
				{
					m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;
					//�������ȱ�־��λ
					m_u16ProgramPorcess = PROGRESS_IN_RESET;

				}

			}
		}
	}

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
	UINT16 u16DataLen;

	//for test
	printf("Flash update: Send one block data1 \n");

	//ȡ��ǰ��Ҫ���������
	UINT16 *pu16Ptr = (UINT16*)(m_ucSectionBuff + m_u16CurrentBlockNum * BLOCK_SIZE * 2);
	pu16Ptr = pu16Ptr + m_u16CurrentFrameNum * 3;


	//������Э��㽻������Ϣ
	m_XmitMsg.ucRsRq = RS_MSG;

	//���ݷ��Ͳ���Ҫ�����Ӧ,�����ǰ״̬�����ڷ�������ʱ
	//���˴����ݶ���
	if ((STATUS_BLOCK_HEAD_OK != m_pHostModuleItc->u16FlashupdateStatus)
		&& (STATUS_BLOCK_DATATRANS_WAITING != m_pHostModuleItc->u16FlashupdateStatus))
	{
		return ucErrCode;
	}


	//��ǰBLOCK�����һ֡
	if (m_u16CurrentFrameNum >= (m_u16FrameNumOneBlock - 1))
	{
		//�ֽ���
		m_XmitMsg.u16Length = m_u16LastFrameWordNumOneBlock * 2;

		for (UINT16 i = 0; i<m_u16LastFrameWordNumOneBlock; i++)
		{
			*((UINT16 *)(m_XmitMsg.pData + i * 2)) = *(pu16Ptr + i);
		}

		//�ȴ�����һ��BLOCK
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_END;
	}

	else
	{
		//�ֽ���
		m_XmitMsg.u16Length = NON_LAST_FRAME_WORD_NUM_ONE_BLOCK * 2;

		*((UINT16 *)(m_XmitMsg.pData + 0)) = *pu16Ptr;
		*((UINT16 *)(m_XmitMsg.pData + 2)) = *(pu16Ptr + 1);
		*((UINT16 *)(m_XmitMsg.pData + 4)) = *(pu16Ptr + 2);
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_WAITING;
		m_u16CurrentFrameNum++;
	}

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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);

	//for test
	printf("Flash update: Recv one block data respond1\n");
	/*
	//����յ��ķ�������Ӧ����Ϣ�����˳�,��δ���TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/
	//������Ч����,׼����������Ϣ
	if (BLOCK_RECV_DATA_OK == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
			TempMsg.ucServiceCode = BLOCK_DATA_SRVCODE;
			TempMsg.ucMsgClass = m_ucMsgClass;
			TempMsg.ucFrag = NONFRAG_MSG;
			TempMsg.ucRsRq = RS_MSG;
			TempMsg.ucSourceId = MAC_ID_MON;
			CAN_MSG_Xmite(&TempMsg);

			//			m_u16ResendCnt = BLOCK_RESEND_CNT;

			//for test
			printf("Flash update: Recv one block data respond=0\n");
		}

		//wait othger node repond
		else
		{
			;//do nothing
		}

	}

	//����ʧ��TBD
	else
	{
		ResendOneBlock();
	}
}



/**********************************************************************
BlockChecksumXmitFcb-----�´�BLOCKУ�������

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockChecksumXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;

	//for test
	printf("Flash update: send one block checksum=%x \n", m_u32CheckSumOneBlock);


	//������Э��㽻������Ϣ
	m_XmitMsg.ucRsRq = RS_MSG;
	//����6�ֽ���Ϣ
	m_XmitMsg.u16Length = 4;

	//��ָ��m_XmitMsg.pDataҲָ��Ҫ���͵�����
	//������Դȡ�Ժ�̨�´���DATA TBD
	//����һ��ָ��
	*((UINT16 *)(m_XmitMsg.pData + 0)) = (UINT16)(m_u32CheckSumOneBlock & 0x0ffff);
	*((UINT16 *)(m_XmitMsg.pData + 2)) = (UINT16)((m_u32CheckSumOneBlock >> 16) & 0x0ffff);

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_WAITING;

	//�����ط���ʱ��
	WaitBlockChksumRespondTimerPost();

	//��ʱ����������
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[6])
	{
		m_u16TimerExpiredCnt[6] = RESEND_WAITING_START_CNT;
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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	//for test
	printf("Flash update: Recv one block checksum respond1\n");

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;
	/*
	//����յ��ķ�������Ӧ����Ϣ�����˳�,��δ���TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/

	//������Ȳ��Ϸ�������˴ν��յ���Ӧ���ź�
	if (PROGRESS_IN_HEADRESPOND_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//������Ч����,׼����������Ϣ
	if (CHECK_SUM_SUCCESFUL == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_OK;
			m_u16ResendCnt = BLOCK_RESEND_CNT;

			//�ر��ط���ʱ��
			WaitBlockChksumRespondTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[6] = RESEND_WAITING_RESET_CNT;

			//�������ȱ�־
			m_u16ProgramPorcess = PROGRESS_IN_BLOCKCHECK_OK;

			//for test
			printf("Flash update: Recv one block checksum respond=0x82\n");
		}

		//wait othger node repond
		else
		{
			;//do nothing
		}
	}

	//У��ʧ��,���ط�TBD
	else
	{
		//�ر��ط���ʱ��
		WaitBlockChksumRespondTimerFlush();

		//reset task
		FlashupdateTaskReset();

		//reset expired cnt
		m_u16TimerExpiredCnt[6] = RESEND_WAITING_RESET_CNT;

		//		m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_OK;

		//�������ȱ�־,�ط��������BLOCKͷӦ��
		m_u16ProgramPorcess = PROGRESS_IN_PROG_ENA_OK;

		ResendOneBlock();
	}
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


	//������Э��㽻������Ϣ
	m_XmitMsg.ucRsRq = RS_MSG;
	//����6�ֽ���Ϣ
	m_XmitMsg.u16Length = 0;

	//��ָ��m_XmitMsg.pDataҲָ��Ҫ���͵�����
	//������Դȡ�Ժ�̨�´���DATA TBD
	//����һ��ָ��


	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_PROGRAM_WAITING;

	//�����ط���ʱ��
	WaitBlockProgramRespondTimerPost();

	//��ʱ����������
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[7])
	{
		m_u16TimerExpiredCnt[7] = RESEND_WAITING_START_CNT;
	}

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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	UINT16 j;

	//for test
	printf("Flash update: Recv program status respond1\n");

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;
	/*
	//����յ��ķ�������Ӧ����Ϣ�����˳�,��δ���TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/

	//������Ȳ��Ϸ�������˴ν��յ���Ӧ���ź�
	if (PROGRESS_IN_BLOCKCHECK_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//������Ч����,׼����������Ϣ
	if (BLOCK_RECV_DATA_OK == u16RetrunStatus)
	{

		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			//�·���ȡFLASHУ����Ϣ
			TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
			TempMsg.ucServiceCode = VERIFY_SRVCODE;
			TempMsg.ucMsgClass = m_ucMsgClass;
			TempMsg.ucFrag = NONFRAG_MSG;
			TempMsg.ucRsRq = RS_MSG;
			TempMsg.ucSourceId = MAC_ID_MON;
			CAN_MSG_Xmite(&TempMsg);

			//DSP��������BLOCK��̳ɹ�
			//׼������FLASH����У��״̬
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;

			//�ر��ط���ʱ��
			WaitBlockProgramRespondTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[7] = RESEND_WAITING_RESET_CNT;

			//�������ȱ�־
			m_u16ProgramPorcess = PROGRESS_IN_FLASH_PROG_OK;

			//			m_u16ResendCnt = BLOCK_RESEND_CNT;
		}

		//wait othger node repond
		else
		{
			;//do nothing
		}

	}

	//����BLOCK���ʧ��,���ط�TBD
	else
	{
		//
		//ResendOneBlock();
		//--20100408:ֱ���˳�--
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

		//�������ȱ�־��λ
		m_u16ProgramPorcess = PROGRESS_IN_RESET;

		//���ظ���̨ʧ����Ϣ
		for (j = 0; j<11; j++)
		{
			m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
		}

		for (j = 0; j<TIMER_CNT_LEN; j++)
		{
			m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
		}
	}

}

/**********************************************************************
NewPeriodicWaitMsgGen-----��ʱ�ȴ����򽫲����������

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
/*
INT32 CAN_FlashupdateMsgHandle::NewPeriodicWaitMsgGen(TimerObserver *pTimerObj)
{
	INT32 i, j;
	CAN_XMIT_QUEUE_MSG_T TempMsg;


	for (i = 0; i < TIMER_CNT_LEN; i++)
	{
		if (pTimerObj == PeriodicSendEraseCmdObjs[i].pTimer)
		{
			//��ʱ�˳�����,���ұ�ʧ��
			//������������ۼ�,û����Ҳ���ط�
			if (m_u16TimerExpiredCnt[i])
			{
				m_u16TimerExpiredCnt[i]++;

				if (m_u16TimerExpiredCnt[i] > RESEND_WAITING_MAX_CNT)
				{
					//��������Ӧ��ʱ��
					if (0 == i)
					{
						//ȡ�ú�ָ̨�������������޷�Ӧ��ģ�飬��Ϊ������
						UINT16 u16NoRespondModule = (m_pHostModuleItc->u16ModIdx ^ m_u16RespondModuleFlag);

						//��·����Ӧj=0,j=1~10 module1~10
						//����Ӧ��������̨��ʾָ����ģ�鲻����
						for (j = 0; j <11; j++)
						{
							if (((u16NoRespondModule >> j) & 1) == 1)
							{
								m_pHostModuleItc->u16UpdateStatus[j] = STATUS_UPDATE_NODE_OFF_LINE;
							}

						}

						//ȡ�ÿ���������ģ��
						m_pHostModuleItc->u16ModIdx &= m_u16RespondModuleFlag;

						//���к�ָ̨����ģ��������������˳�
						if (0 == m_pHostModuleItc->u16ModIdx)
						{
							m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

							//�������ȱ�־��λ
							m_u16ProgramPorcess = PROGRESS_IN_RESET;

							//���ظ���̨ʧ����Ϣ
							for (j = 0; j<11; j++)
							{
								m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
							}

							for (j = 0; j<TIMER_CNT_LEN; j++)
							{
								m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
							}
						}

						//��ָ̨����ģ�����������������״̬��ת����һ��״̬
						else
						{
							ResetFlsUpdateMoudele();
							FlashupdateNodeGet();

							//�·���ȡоƬ������Ϣ
							TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
							TempMsg.ucServiceCode = CHIP_DECODE_SRVCODE;
							TempMsg.ucMsgClass = m_ucMsgClass;
							TempMsg.ucFrag = NONFRAG_MSG;
							TempMsg.ucRsRq = RS_MSG;
							TempMsg.ucSourceId = MAC_ID_MON;
							CAN_MSG_Xmite(&TempMsg);

							//�ȴ�������Ϣ����,�����ȴ���ʱTBD
							m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_CHIP_DECODE;

							//�������ȱ�־
							m_u16ProgramPorcess = PROGRESS_IN_HAND_OK;

							//�����ֶ�ʱ��
							WaitHandleTimerFlush();

							FlashupdateTaskReset();

							m_u16TimerExpiredCnt[0] = RESEND_WAITING_RESET_CNT;

						}



					}
					//������ʱ
					else
					{
						m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

						//�������ȱ�־��λ
						m_u16ProgramPorcess = PROGRESS_IN_RESET;

						//���ظ���̨ʧ����Ϣ
						for (j = 0; j<11; j++)
						{
							m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
						}

						for (j = 0; j<TIMER_CNT_LEN; j++)
						{
							m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
						}

						//close
						//FlashUpdateTimerFlush(pTimerObj);
					}
				}

				else
				{
					TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
					TempMsg.ucServiceCode = PeriodicSendEraseCmdObjs[i].ucServiceCode;
					TempMsg.ucMsgClass = m_ucMsgClass;
					TempMsg.ucFrag = NONFRAG_MSG;
					TempMsg.ucRsRq = RS_MSG;
					TempMsg.ucSourceId = MAC_ID_MON;
					CAN_MSG_Xmite(&TempMsg);

					//������ʱ��
					FlashUpdateTimerPost(pTimerObj);

					//for test
					printf("Flash update: Repost a timer,ucServiceCode=%d\n", PeriodicSendEraseCmdObjs[i].ucServiceCode);
				}

				break;
			}

			else
			{
				;//do nothing
			}


		}
	}

	return 0;
}
*/


/**********************************************************************
TimeConvert-----��

Parameters:
Return Value:
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::TimeConvert(OUT DATE_TIME_T * pCalendar,
	IN INT32 i32Seconds,
	IN UINT16 u16MilliSeconds)
{
	struct tm *ptNowTime = NULL;
	struct timeval m_tTimeCur;
	time_t now;

	//time(&(time_t)i32Seconds);
	now = (time_t)i32Seconds;
	//	ptNowTime = localtime(&((time_t)i32Seconds));
	ptNowTime = localtime(&now);

	pCalendar->usMiliSecs = u16MilliSeconds;

	pCalendar->ucSeconds = (UCHAR)(ptNowTime->tm_sec);
	pCalendar->ucMinutes = (UCHAR)(ptNowTime->tm_min);
	pCalendar->ucHours = (UCHAR)(ptNowTime->tm_hour);
	pCalendar->ucDates = (UCHAR)(ptNowTime->tm_mday);
	pCalendar->ucMonths = (UCHAR)(ptNowTime->tm_mon + 1);

	pCalendar->ucCentraries = OS_BASE_CENTARY + (UCHAR)(ptNowTime->tm_year) / 100;

	pCalendar->ucYears = (UCHAR)((OS_BASE_YEAR + ptNowTime->tm_year) % 100);

	pCalendar->ucWeeks = (UCHAR)(ptNowTime->tm_wday + 1);
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
	UCHAR i, ucRet, j;
	UCHAR ucUpdateCnt;

	ucUpdateCnt = 0;

	//Ĭ��Ϊ�㲥
	m_u16UpdaingNodeAdd = 0x3f;
	ucRet = 0;
	for (i = 0; i<FLASHUPDATE_NODE_CNT; i++)
	{


		//---------------------------------------------------------------------------------
		//---20100111---
		if (m_pHostModuleItc->sFlashUpdateFlag == ucFlashUpdateNodeObj[i].ucFlashObj)
		{
			if (ucFlashUpdateNodeObj[i].u16FlashModule == ((m_pHostModuleItc->u16ModIdx)&(ucFlashUpdateNodeObj[i].u16FlashModule)))
			{
				m_u16UpdaingNodeAdd = ucFlashUpdateNodeObj[i].u16DestNodeAdd;

				//m_pHostModuleItc->u16ModIdx &= (~(ucFlashUpdateNodeObj[i].u16FlashModule));

				//for test
				//				printf("Dest Module:ModuleNum=i%d,m_u16UpdaingNodeAdd=%x,u16ModIdx=%x\n",i,m_u16UpdaingNodeAdd,m_pHostModuleItc->u16ModIdx);
				//��������̨״̬��Ϣ

				//������·
				if (0x0F == m_u16UpdaingNodeAdd)
				{
					m_u16UpdatingModuleNo = 0;
					m_tFlashupdateTarged[0].ucTargeAddr = 0x0f;
					m_tFlashupdateTarged[0].ucTargetEnable = TARGET_UPDATE_ENABLE;
					m_tFlashupdateTarged[0].ucTaskHandled = TASK_HANDLE_NON;

					ucUpdateCnt = 0;

					for (j = 1; j<11; j++)
					{
						m_tFlashupdateTarged[j].ucTargetEnable = TARGET_UPDATE_DISABLE;
					}

					break;

				}

				else if ((0x20 <= m_u16UpdaingNodeAdd) && (0x29 >= m_u16UpdaingNodeAdd))
				{
					m_u16UpdatingModuleNo = m_u16UpdaingNodeAdd - 0x20 + 1;

					m_tFlashupdateTarged[m_u16UpdatingModuleNo].ucTargeAddr = m_u16UpdaingNodeAdd;
					m_tFlashupdateTarged[m_u16UpdatingModuleNo].ucTargetEnable = TARGET_UPDATE_ENABLE;
					m_tFlashupdateTarged[m_u16UpdatingModuleNo].ucTaskHandled = TASK_HANDLE_NON;

					ucUpdateCnt++;

				}

				else if ((0x10 <= m_u16UpdaingNodeAdd) && (0x19 >= m_u16UpdaingNodeAdd))
				{
					m_u16UpdatingModuleNo = m_u16UpdaingNodeAdd - 0x10 + 1;

					m_tFlashupdateTarged[m_u16UpdatingModuleNo].ucTargeAddr = m_u16UpdaingNodeAdd;
					m_tFlashupdateTarged[m_u16UpdatingModuleNo].ucTargetEnable = TARGET_UPDATE_ENABLE;
					m_tFlashupdateTarged[m_u16UpdatingModuleNo].ucTaskHandled = TASK_HANDLE_NON;

					ucUpdateCnt++;

				}

				else
				{

				}

				m_pHostModuleItc->u16UpdateStatus[m_u16UpdatingModuleNo] = STATUS_HOST_UPDAT_ING;

				ucRet = 1;

				//break;
			}
		}

		//for test
		//		printf("Dest Module:Ret=%d,m_u16UpdaingNodeAdd=%x,u16ModIdx=%x\n",ucRet,m_u16UpdaingNodeAdd,m_pHostModuleItc->u16ModIdx);

	}

	//ͬʱ�����������Ͻڵ�
	if (ucUpdateCnt > 1)
	{
		m_tFlashupdateTarged[0].ucTargetEnable = TARGET_UPDATE_DISABLE;
		m_u16UpdaingNodeAdd = 0x3f;
	}

	//only one node
	else if (1 == ucUpdateCnt)
	{
		m_tFlashupdateTarged[0].ucTargetEnable = TARGET_UPDATE_DISABLE;
	}

	//do nothing
	else
	{

	}

	return ucRet;

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
SetRespondModuleFlag-----FLASH UPDATE ��Ӧģ��������Ӧ��˵��������

Parameters:

Return Value: N/A
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::SetRespondModuleFlag(UCHAR ucRecvAddr)
{

	//����
	if ((ucRecvAddr >= 0x20) && (ucRecvAddr <= 0x29))
	{
		//		m_u16RespondModuleFlag |= ((ucRecvAddr-0x20)<<1);
		m_u16RespondModuleFlag |= utFlashFlagMaskObj[(ucRecvAddr - 0x20) + 1].u16FlashFlagMask;
	}

	//���
	else if ((ucRecvAddr >= 0x10) && (ucRecvAddr <= 0x19))
	{
		//		m_u16RespondModuleFlag |= ((ucRecvAddr-0x10)<<1);

		m_u16RespondModuleFlag |= utFlashFlagMaskObj[(ucRecvAddr - 0x10) + 1].u16FlashFlagMask;
	}

	//��·
	else if (ucRecvAddr == 0x0f)
	{
		m_u16RespondModuleFlag = 1;
	}

	else
	{
		;//do nothing
	}


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

//-----------------------------------------------------
/////////////////////////////////////////////////////////////////////////
//      ��ȡ�ļ�ͷ


//-----------------------------------------------------




//��һ����������
void CAN_FlashupdateMsgHandle::ReadASection(BYTE *buff, DWORD& pos, WORD& nLength, UINT& nAddress, BYTE *sectionbuff)
{
	while (buff[pos] == 0x0D || buff[pos] == 0x0A || buff[pos] == ' ')
	{
		pos++;
	}

	if (buff[pos] == 0x03 || buff[pos + 7] == 0x03)
	{
		nLength = 0;
		nAddress = 0;

		return;
	}

	if (buff[pos] == 0x02)
		pos += 68;

	WORD2BYTE transfer1;
	transfer1.bytes[0] = ReadAbyte(buff, pos);
	transfer1.bytes[1] = ReadAbyte(buff, pos);
	nLength = transfer1.word;

	if (nLength == 0)
	{
		return;
	}

	UINT2BYTE transfer2;
	transfer2.bytes[2] = ReadAbyte(buff, pos);
	transfer2.bytes[3] = ReadAbyte(buff, pos);
	transfer2.bytes[0] = ReadAbyte(buff, pos);
	transfer2.bytes[1] = ReadAbyte(buff, pos);
	nAddress = transfer2.uint;


	for (int i = 0; i<nLength * 2; i++)
	{
		sectionbuff[i] = ReadAbyte(buff, pos);
	}
}

//UCHAR XXX=0;
BYTE CAN_FlashupdateMsgHandle::ReadAbyte(BYTE *buff, DWORD& pos)
{
	BYTE nData;
	BYTE ucInput1, ucInput2;

	while (buff[pos] == 0x0D || buff[pos] == 0x0A || buff[pos] == ' ')
	{
		pos++;
	}
	/*
	//for test
	struct timeval TimeCurr;

	if(XXX < 10 )
	{
	//Get current time
	gettimeofday(&TimeCurr,NULL);
	printf("Flash time1:s=%d,ms=%d\n",TimeCurr.tv_sec,TimeCurr.tv_usec);
	}
	*/
	//sscanf((char*)buff+pos, "%2X", &nData);
	//nData = TwoAsciiToByte(*(buff+pos),*(buff+pos+1));
	ucInput1 = *(buff + pos);
	ucInput2 = *(buff + pos + 1);
	if (ucInput1 > 0x39)
	{
		ucInput1 = ucInput1 - 0x37;					/*'A' - 'F'*/
	}
	else
	{
		ucInput1 = ucInput1 - 0x30;					/*'0' - '9'*/
	}

	if (ucInput2 > 0x39)
	{
		ucInput2 = ucInput2 - 0x37;					/*'A' - 'F'*/
	}
	else
	{
		ucInput2 = ucInput2 - 0x30;					/*'0' - '9'*/
	}

	nData = (ucInput1 << 4) + ucInput2;
	/*
	if(XXX < 10 )
	{
	//Get current time
	gettimeofday(&TimeCurr,NULL);
	printf("Flash time2:s=%d,ms=%d\n",TimeCurr.tv_sec,TimeCurr.tv_usec);
	}

	if(XXX < 10 )
	{
	XXX++;
	printf("SSCANF:bef1=%x,bef2=%x,aft=%x\n",*(buff+pos),*(buff+pos+1),nData);
	}
	else
	{
	XXX =10;
	}
	*/
	//	nData = *(buff+pos);
	pos += 2;

	return nData;
}

UINT32 CAN_FlashupdateMsgHandle::CheckSum(UINT16 u16Length, UINT32 nAddress, BYTE *buf)
{
	UINT32 R = 0;
	UINT16 wData;

	R += u16Length;

	//	R += nAddress >> 16;
	//	R += nAddress &  0xFFFF;
	R += nAddress;

	for (int i = 0; i<u16Length * 2; i += 2)
	{
		wData = ((UINT16)buf[i + 1] << 8) + buf[i];
		R += wData;
	}

	return R;
}

//BLOCK�ط�����m_u16ResendCnt
/**********************************************************************
ResendOneBlock-----��ȡҪ�����Ľڵ�

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::ResendOneBlock(VOID)
{
	UINT16 j;

	if (m_u16ResendCnt)
	{
		m_u16CurrentFrameNum = 0;
		//״̬���ص�������һ��BLOCKͷ
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
		//for test
		printf("Flash update: Resend a block head,m_u16ResendCnt=%d\n", m_u16ResendCnt);
		m_u16ResendCnt--;

	}

	//�ط���3�α����˳�����TBD
	else
	{
		//״̬���ص�������һ��BLOCKͷ
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

		//�������ȱ�־��λ
		m_u16ProgramPorcess = PROGRESS_IN_RESET;

		//���ظ���̨ʧ����Ϣ
		for (j = 0; j<11; j++)
		{
			m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
		}

		for (j = 0; j<TIMER_CNT_LEN; j++)
		{
			m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
		}

		//��������̨״̬��Ϣ
		m_pHostModuleItc->u16UpdateStatus[m_u16UpdatingModuleNo] = STATUS_HOST_UPDATE_FAIL;

		//for test
		printf("Flash update: Resend overtime,m_u16ResendCnt=%d\n", m_u16ResendCnt);
	}
}








UINT32 CAN_FlashupdateMsgHandle::BlockMessageProcess_Packaged(void)
{

#define RECORD_TYPE_OFFSET	3
#define	DATA_OFFSET	4
#define	HIGH_16BIT_ADDRESS_RECORD_TYPE	0x04
#define	DATA_DEFINICATION	0x00
#define DATA_END_RECORD_TYPE	0x01
	if (IDNO == AfxMessageBox(_T("Are you sure Flashupdate?"), MB_YESNO))return;
	//WinExec("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out", SW_NORMAL);
	//system("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out");
	//system("del 28377D_INV.hex");

	CFile	file;
	CFile	wfile;
	CFileException ex;
	CString filepath;
	UINT16	check_every_block_size;
	BYTE	check_low16_address;

	GetDlgItemText(IDC_MFCEDITBROWSE1, filepath);
	if (!file.Open(filepath, CFile::modeRead | CFile::shareDenyWrite, &ex)) {

		MessageBox(_T("����������ļ���"), _T("����"), MB_OK | MB_ICONQUESTION);
		return;
	}
	DWORD file_length = (DWORD)file.GetLength();
	char *p = new char[file_length];
	file.Read(p, file_length);
	file.Close();
	while (transfer_data.next_data_head < file_length) {
		for (DWORD i = transfer_data.next_data_head; i < file_length; ++i) {

			if (p[i] == ':') {
				transfer_data.data_head = i + 1;
			}
			if (p[i] == '\r') {
				transfer_data.data_trail = i - 1;
				transfer_data.next_data_head = i + 2;
				transfer_data.data_count = transfer_data.data_trail - transfer_data.data_head + 1;
				break;
			}

		}
		// Record Type( 00, 04, 01)
		if (p[transfer_data.data_head + RECORD_TYPE_OFFSET] == HIGH_16BIT_ADDRESS_RECORD_TYPE) {

			BlockAddress[BlockCount][1] = (((UINT16)(p[transfer_data.data_head + 4])) << 8) |
											(((UINT16)(p[transfer_data.data_head + 5])));
			
		}
		else if(p[transfer_data.data_head + RECORD_TYPE_OFFSET] == DATA_END_RECORD_TYPE){

			break;

		}
		else {

			check_every_block_size = EveryBlockDataNum[BlockCount] + (UINT16)p[transfer_data.data_head];
			if (BlockCount == 0) {

				BlockAddress[BlockCount][0] = (((UINT16)(p[transfer_data.data_head + 1])) << 8) |
												(((UINT16)(p[transfer_data.data_head + 2])));
			}
			if (check_every_block_size > 2048{
				BlockCount++;
				BlockAddress[BlockCount][0] = (((UINT16)(p[transfer_data.data_head + 1])) << 8) |
												(((UINT16)(p[transfer_data.data_head + 2])));
			}
			else if (check_low16_address != p[transfer_data.data_head + 2]) {

				BlockCount++;
				BlockAddress[BlockCount][0] = (((UINT16)(p[transfer_data.data_head + 1])) << 8) |
												(((UINT16)(p[transfer_data.data_head + 2])));
			}
			for (int j = transfer_data.data_head + DATA_OFFSET; j < transfer_data.data_trial; ++j) {

				if (j % 2) {

					BlockData[BlockCount][EveryBlockDataNum[BlockCount]] |= ((UINT16)p[j])<<8;
					EveryBlockDataNum[BlockCount]++;
				}
				else {

					BlockData[BlockCount][EveryBlockDataNum[BlockCount]] = (UINT16)p[j];
				}
			}

		}

		check_low16_address = p[transfer_data.data_head] + p[transfer_data.data_head+2];
	}

	delete[]p;
}
