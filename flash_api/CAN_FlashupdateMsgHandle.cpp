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

#define SET_EVENT 						0x5555		//当前记录类型
#define CLEAR_EVENT 						0x1111		//历史记录类型

#define SET_PARA_SUPPORT 1
#define SET_PARA_NOT_SUPPORT 0

#define COMM_FAIL_TIMEOUT_CNT 5


//发送、接收处理函数定义
typedef INT32(CAN_FlashupdateMsgHandle::*_MSG_XMIT_FCB)(VOID);
typedef INT32(CAN_FlashupdateMsgHandle::*_MSG_RECV_FCB)(VOID);

typedef struct
{
	UCHAR ucServiceCode;
	_MSG_RECV_FCB pMsgXmitFcb;
	_MSG_RECV_FCB pMsgRecvFcb;
}_CAN_FLASHUPDATE_MSG_T;



//读取HEX文件结束标志定义
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
	//模块号.0--旁路,1~10对应模块1~10
	UINT16 u16FlashModule;
	UINT16 u16FlashFlagMask;
}_FLASH_FLASH_MASK_T;

//模块握手有无应答处理相关标志
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

//一个BLOCK按1024个字来算,DSP侧开辟的缓冲区最大为1024字
#define BLOCK_SIZE	1024

//一个CAN帧最多传输有效字数为3(6字节)
#define FRAME_VALID_DATA_SIZE		3

//一个完整的BLOCK(1024字)需341帧(3个有效字)+1帧(1个有效字)=1024字
#define FRAME_ALL_NUM_ONE_BLOCK	342
//一个完整的BLOCK所有帧中最后一帧的字的个数
#define LAST_FRAME_WORD_NUM_ONE_BLOCK 1
//一个完整的BLOCK所有帧中非最后一帧的字的个数
#define NON_LAST_FRAME_WORD_NUM_ONE_BLOCK 3

//BLOCK 重发次数
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



	//初始化升级目标变量
	for (i = 0; i<11; i++)
	{
		m_tFlashupdateTarged[i].ucTargeAddr = 0xff;
		m_tFlashupdateTarged[i].ucTargetEnable = TARGET_UPDATE_DISABLE;
		m_tFlashupdateTarged[i].ucTaskHandled = TASK_HANDLED;
	}

	m_u16RespondModuleFlag = 0;

	//升级进度标志
	m_u16ProgramPorcess = PROGRESS_IN_RESET;

	m_pHostModuleItc = new _HOST_MODULE_ITC_T;
	m_XmitMsg.pData = m_ucXmitMsgBuf;
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
	
	UCHAR ucStateFlag;

	ucStateFlag = 0;

	//for test
	//static UCHAR ucTestFlag=0;

	//for test
	//	FlashupdateNodeGet();
	//	return;

	//置FLASH UDATE 状态机启动

	switch (m_pHostModuleItc->u16FlashupdateStatus)
	{
		//开始升级
		//获取升级对象
	case STATUS_FLASH_START:

		//升级进度标志
		m_u16ProgramPorcess = PROGRESS_IN_START;

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
		HandCommRecvFcb();
		break;
		//等待芯片解密信号
	case STATUS_WAITING_CHIP_DECODE:
		//do nothing
		break;
		//等待API版本确认信息
	case STATUS_WAITING_API_VERSION:
		//do nothing
		break;
		//API VERSION OK
	case STATUS_API_OK:
		//下发擦除命令
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = ERASE_SECTOR_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);

		break;
		//等待擦除命令应答
	case STATUS_FLASH_ERASE_GOING:

		break;
		//擦除成功
	case STATUS_FLASH_ERASED:
		//下发编程请求命令
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = PROGRAM_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		break;
		//等待编程允许状态
	case STATUS_PROGRAM_PERMIT_WAITING:
		//do nothing
		break;
		//编程允许
	case STATUS_PROGRAM_ENABLE:
		//读section
		//for test
		printf("Flash update: Read a section from target file\n");
		//--------------------------------------
		//首先读一个Section相关信息
		//从总的缓冲区中指定位置开始读出一个BLOCK的信息
		//首先要计算BLOCK的大小
		//ReadASection(m_byBuff, m_ulPos, m_wSectionLen, m_uSectionAddress, m_ucSectionBuff);
		ReadASection(cFlashUpdateBuf, m_ulPos, m_wSectionLen, m_uSectionAddress, m_ucSectionBuff);

		printf("Read section:m_ulPos=%x, m_wSectionLen=%x,m_uSectionAddress=%x\n", m_ulPos, m_wSectionLen, m_uSectionAddress);

		printf("Flash update: Read a section from target file--over\n");
		//-----------------------------------------------------
		/*
		//only for test
		if (0 == ucTestFlag)
		{
		for (UINT16 j=0; j<72;j++)
		m_ucSectionBuff[j] = ucFlashUpdateTestData[j];

		m_wSectionLen = 36;
		m_uSectionAddress = 0x3e8100;
		ucTestFlag = 1;
		}
		else
		{
		m_wSectionLen = 0;
		}
		*/

		//------------------------------------------------------

		//计算本次SECTION共划分多少个BLOCK
		m_u16BlockNumOfSection = m_wSectionLen / BLOCK_SIZE;

		//section 中剩余的字数
		if (m_wSectionLen>(m_u16BlockNumOfSection*BLOCK_SIZE))
		{
			m_u16RemainWordOfSection = m_wSectionLen - m_u16BlockNumOfSection * BLOCK_SIZE;
			//剩余BLOCK的帧数
			if (m_u16RemainWordOfSection % 3)
			{
				m_u16FrameNumOfRemainBlock = m_u16RemainWordOfSection / 3 + 1;
				//剩余BLOCK中最后一帧字的个数
				m_u16LastFrameWordNumOfRemainBlock = m_u16RemainWordOfSection - (m_u16RemainWordOfSection / 3) * 3;
			}

			else
			{
				m_u16FrameNumOfRemainBlock = m_u16RemainWordOfSection / 3;
				//剩余BLOCK中最后一帧字的个数
				m_u16LastFrameWordNumOfRemainBlock = NON_LAST_FRAME_WORD_NUM_ONE_BLOCK;
			}


			//本次SECTION总共的BLOCK数
			m_u16BlockNumOfSection = m_u16BlockNumOfSection + 1;
		}

		//结束,长度为0
		else if (0 == m_wSectionLen)
		{
			m_u16RemainWordOfSection = 0;
			m_u16FrameNumOfRemainBlock = 0;

			//置读取HEX文件结束标志
			m_u16ReadHexFileEnd = READ_HEX_END_FLAG_VALID;

		}

		//整除且长度不为0,则最后一个BLOCK也是1024
		else
		{
			m_u16RemainWordOfSection = BLOCK_SIZE;
			m_u16FrameNumOfRemainBlock = FRAME_ALL_NUM_ONE_BLOCK;
			m_u16LastFrameWordNumOfRemainBlock = LAST_FRAME_WORD_NUM_ONE_BLOCK;
		}

		/*
		//本次SECTION 超过了1024字
		if (m_u16BlockNumOfSection > 0)
		{
		//一个完整的BLOCK需要分的帧数
		m_u16FrameNumOneBlock = FRAME_ALL_NUM_ONE_BLOCK;

		}

		else
		{
		m_u16FrameNumOneBlock = m_u16FrameNumOfRemainBlock;
		}
		*/
		//首次的BLOCK地址就是SECTION地址
		m_uBlockAddress = m_uSectionAddress;


		//每读一次SECTON,当前BLOCK都应该初始化成0
		//其范围为[0,m_u16BlockNumOfSection),其中最大值m_u16BlockNumOfSection为一个少于1024字的BLOCK
		m_u16CurrentBlockNum = 0;


		//m_u16FrameNumOneBlock = m_wBlockLen/3;
		//m_u16LastFrameWordNumOneBlock = m_u16FrameNumOneBlock*3;

		//m_u16CurrentFrameNum范围为0~m_u16FrameNumOneBlock
		m_u16CurrentFrameNum = 0;

		//--------------------------------------

		//下发BLOCK头
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_HEAD_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;

		CAN_MSG_Xmite(&m_XmitMsg);

		m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_WAITING;
		break;

		//头BLOCK传输中
	case STATUS_BLOCK_HEAD_WAITING:
		//do nothing
		break;
		//下一个BLOCK头
	case STATUS_NEXT_BLOCK_HEAD:
		//下发BLOCK头
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_HEAD_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		break;

		//头传输结束
	case STATUS_BLOCK_HEAD_OK:
		//BLOCK数据传输过程中
	case STATUS_BLOCK_DATATRANS_WAITING:
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_DATA_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		break;

		//BLOCK数据传输结束
	case STATUS_BLOCK_DATATRANS_END:
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_CHECKSUM_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);
		break;
		//block校验中
	case STATUS_BLOCK_CHECKSUM_WAITING:
		//do nothing
		break;
		//BLOCK校验OK
	case STATUS_BLOCK_CHECKSUM_OK:
		//获取对应的编程状态命令
		m_XmitMsg.u16DestinationId = m_u16UpdaingNodeAdd;
		m_XmitMsg.ucServiceCode = BLOCK_PROMG_STATUS_SRVCODE;
		m_XmitMsg.ucMsgClass = m_ucMsgClass;
		m_XmitMsg.ucFrag = NONFRAG_MSG;
		m_XmitMsg.ucRsRq = RS_MSG;
		m_XmitMsg.ucSourceId = MAC_ID_MON;
		CAN_MSG_Xmite(&m_XmitMsg);

		break;
		//编程状态
	case STATUS_BLOCK_PROGRAM_WAITING:
		//DO NOTHING
		break;

		//编程完毕
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
		//文件传输结束
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

		//校验中
	case STATUS_FLASH_VERIFYING:
		//do nothing
		break;
		//校验完毕
	case STATUS_FLASH_UPDATE_OVER:


		//状态机复位
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_INVALID;

		//升级进度标志复位
		m_u16ProgramPorcess = PROGRESS_IN_RESET;
		break;
	default:

		//升级进度标志复位
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
Msg_Xmit-----消息发送处理


Parameters:
pCanXmitMsg:应用层正在发送消息内容
Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::Msg_Xmit(IN CAN_XMIT_QUEUE_MSG_T * pCanXmitMsg)
{
	INT32 i, ret;
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;


	//更新当前发送数据
	m_XmitQueueMsg = *pCanXmitMsg;

	//根据RS/RE标志，升级与协议层交互的信息
	m_XmitMsg.ucServiceCode = m_XmitQueueMsg.ucServiceCode;
	m_XmitMsg.ucSourceId = MAC_ID_MON;
	m_XmitMsg.ucDestinationId = (m_XmitQueueMsg.u16DestinationId & 0x00ff);
	m_XmitMsg.u16Length = 0;

	//查找对应的处理函数，并执行
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
Msg_Recv-----消息接收处理

Parameters:
pCanRecvMsg:由协议层上传的消息内容
Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::Msg_Recv(CAN_APDU_T *pCanRecvMsg)
{
	INT32 i;

	//更新当前发送数据
	m_RecvMsg = *pCanRecvMsg;

	//查找对应的处理函数，并执行
	for (i = 0; i < CAN_FLASHUPDATE_MSG_OBJ_CNT; i++)
	{
		//增加接收的条件---比如对于升级整流时只能接收整流返回帧
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
HandCommXmitFcb-----握手信号下发

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::HandCommXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;
	int device_type = 4;	// CANalyst-II
	int device_ind = 0;		// first device
	int can_ind = 0;		// CAN channel 0
	CAN_PACKED_PROTOCOL_U	tx_msg;

	//发握手命令

	tx_msg.PackedMsg.RemoteFlag = 0;// 数据帧
	tx_msg.PackedMsg.ExternFlag = 0;// 标准帧

	tx_msg.PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;// 填写第一帧的ID
	tx_msg.PackedMsg.b7ServiceCode = HANDS_COMM_SRVCODE;// 正常发送
	tx_msg.PackedMsg.b10MsgClass = m_ucMsgClass;
	tx_msg.PackedMsg.b1Fragment = NONFRAG_MSG;
	tx_msg.PackedMsg.b1RsRq = RS_MSG;
	tx_msg.PackedMsg.b6SourceMacId = MAC_ID_MON;
	tx_msg.PackedMsg.DataLen = 4;// 数据长度4个字节
	tx_msg.PackedMsg.MsgData[0] = HAND_COMM_QUERY & 0xFF;
	tx_msg.PackedMsg.MsgData[1] = 0xFF & (HAND_COMM_QUERY >> 8);

	VCI_Transmit(device_type, device_ind, can_ind, &tx_msg.Frame, 1);


	//初始化数据
	m_ulPos = 0;
	m_uBlockAddress = 0;
	m_uSectionAddress = 0;
	m_wBlockLen = 0;

	m_u16ResendCnt = BLOCK_RESEND_CNT;


	//等待握手应答信号
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_HANDS_RESPOND;

	//反馈给后台状态信息
	m_pHostModuleItc->u16UpdateStatus[m_u16UpdatingModuleNo] = STATUS_HOST_UPDAT_ING;
	return ucErrCode;
}

/**********************************************************************
HandCommRecvFcb-----握手信号接收

Parameters:		=0xaa表示握手成功

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::HandCommRecvFcb(VOID)
{
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;

	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	/*
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	return 1;
	}
	*/
	//如果进度不在等待握手应答则放弃此次接收到的握手信号
	if (PROGRESS_IN_START != m_u16ProgramPorcess)
	{
		return 1;
	}

	//握手成功
	if (HAND_OK_RESPOND == u16RetrunStatus)
	{
		//置相应模块回应标志
		SetRespondModuleFlag(m_RecvMsg.ucSourceId);


		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			//下发获取芯片解密信息
			TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
			TempMsg.ucServiceCode = CHIP_DECODE_SRVCODE;
			TempMsg.ucMsgClass = m_ucMsgClass;
			TempMsg.ucFrag = NONFRAG_MSG;
			TempMsg.ucRsRq = RS_MSG;
			TempMsg.ucSourceId = MAC_ID_MON;
			CAN_MSG_Xmite(&TempMsg);

			//等待解密信息反馈,启动等待延时TBD
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_CHIP_DECODE;

			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_HAND_OK;
		}
		//wait othger node repond
		else
		{
			;//do nothing
		}

	}
	//擦除失败,置反馈状态字给后台TBD
	else
	{
		;//
	}


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


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;
	//传送2字节
	m_XmitMsg.u16Length = 0;

	//将指针m_XmitMsg.pData也指向将要发送的数据
	//*((UINT16 *)(m_XmitMsg.pData + 0)) = HAND_COMM_QUERY;

	//启动一个超时计数器,超时后重发,可重发3次,
	//如果重发三次失败则反馈给后台,告知升级失败
	//TBD

	//启动定时器
	//TimerPost();
	WaitCSMTimerPost();

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[1])
	{
		m_u16TimerExpiredCnt[1] = RESEND_WAITING_START_CNT;
	}

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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;

	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	/*	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	return 1;
	}
	*/

	//如果进度不在等待解密应答则放弃此次接收到的应答信号
	if (PROGRESS_IN_HAND_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//芯片解密OK
	if (CHIP_DECODE_SUCCESS == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			//下发获取API版本信息
			TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
			TempMsg.ucServiceCode = API_VERSION_SRVCODE;
			TempMsg.ucMsgClass = m_ucMsgClass;
			TempMsg.ucFrag = NONFRAG_MSG;
			TempMsg.ucRsRq = RS_MSG;
			TempMsg.ucSourceId = MAC_ID_MON;
			CAN_MSG_Xmite(&TempMsg);

			//等待API版本核对信息反馈,启动等待延时TBD
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_API_VERSION;

			//启动定时器
			//TimerPost();
			//WaitApiTimerPost();

			//关芯片解密等待定时器
			WaitCSMTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[1] = RESEND_WAITING_RESET_CNT;

			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_DECODE_OK;

		}

		//wait othger node repond
		else
		{
			;//do nothing
		}
	}

	//擦除失败,置反馈状态字给后台TBD
	else
	{
		;//
	}


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


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;
	//传送2字节
	m_XmitMsg.u16Length = 0;

	//将指针m_XmitMsg.pData也指向将要发送的数据
	//*((UINT16 *)(m_XmitMsg.pData + 0)) = HAND_COMM_QUERY;

	//启动一个超时计数器,超时后重发,可重发3次,
	//如果重发三次失败则反馈给后台,告知升级失败
	//TBD

	//启动定时器
	//TimerPost();
	WaitApiTimerPost();

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[2])
	{
		m_u16TimerExpiredCnt[2] = RESEND_WAITING_START_CNT;
	}

	//FlashupdateTaskReset();

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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;

	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	/*	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	return 1;
	}
	*/

	//如果进度不在等待API版本应答则放弃此次接收到的应答信号
	if (PROGRESS_IN_DECODE_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//API版本核对成功
	if (API_VESION_OK == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_API_OK;

			//关闭定时器
			WaitApiTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[2] = RESEND_WAITING_RESET_CNT;

			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_APIVERSION_OK;

		}

		//wait othger node repond
		else
		{
			;//do nothing
		}
	}

	//擦除失败,置反馈状态字给后台TBD
	else
	{
		;//
	}


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
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;
	//传送2字节擦除指定扇区
	m_XmitMsg.u16Length = 2;

	//将指针m_XmitMsg.pData也指向将要发送的数据
	if (0x95 == m_pHostModuleItc->u16FlashUpdateKernelFlag)
	{
		*((UINT16 *)(m_XmitMsg.pData + 0)) = ERASE_SECTOR_ALL;
	}

	else
	{
		*((UINT16 *)(m_XmitMsg.pData + 0)) = ERASE_SECTOR_BCD;
	}

	//启动一个超时计数器,超时后重发,可重发3次,
	//如果重发三次失败则反馈给后台,告知升级失败
	//TBD

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_ERASE_GOING;

	//启动定时器
	//TimerPost();
	WaitEraseTimerPost();

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[3])
	{
		m_u16TimerExpiredCnt[3] = RESEND_WAITING_START_CNT;
	}

	return ucErrCode;
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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;

	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	/*	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	return 1;
	}
	*/

	//如果进度不在等待API版本应答则放弃此次接收到的应答信号
	if (PROGRESS_IN_APIVERSION_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//擦除成功,准备发编程命令
	if (ERASE_SUCCESFULL == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_ERASED;

			//关闭定时器
			WaitEraseTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[3] = RESEND_WAITING_RESET_CNT;

			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_ERASE_OK;

		}

		//wait othger node repond
		else
		{
			;//do nothing
		}
	}

	//擦除失败TBD
	else
	{
		;//
	}


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
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;

	m_XmitMsg.u16Length = 0;

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_PROGRAM_PERMIT_WAITING;

	//启动定时器
	//TimerPost();
	WaitProgramEnableTimerPost();

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[4])
	{
		m_u16TimerExpiredCnt[4] = RESEND_WAITING_START_CNT;
	}

	return ucErrCode;
}

/**********************************************************************
ProgramRecvFcb-----编程命令反馈

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::ProgramRecvFcb(VOID)
{
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	UINT16 j;

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);
	//for test
	printf("Flash update: Recv program permit1,\n");

	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	/*	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//for test
	printf("Flash update: Recv program permit12,\n");
	//return 1;
	ResendOneBlock();

	}
	*/

	//如果进度不合法则放弃此次接收到的应答信号
	if (PROGRESS_IN_ERASE_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//允许编程,准备发数据头信息
	if (PROGRAM_ENABLE == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_PROGRAM_ENABLE;

			//			m_u16ResendCnt = BLOCK_RESEND_CNT;

			//关闭定时器
			WaitProgramEnableTimerFlush();

			//for test
			printf("Flash update: Recv program permit succesful,\n");

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[4] = RESEND_WAITING_RESET_CNT;

			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_PROG_ENA_OK;
		}

		//wait othger node repond
		else
		{
			;//do nothing
		}
	}

	//擦除失败TBD
	else
	{
		//for test
		printf("Flash update: Recv program permit123,\n");
		//ResendOneBlock();
		//--20100408:直接退出--
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;
		//升级进度标志复位
		m_u16ProgramPorcess = PROGRESS_IN_RESET;

		//返回给后台失败信息
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
VerifyXmitFcb-----校验命令下发

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::VerifyXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;

	m_XmitMsg.u16Length = 0;
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;

	//启动重发定时器
	WaitBlockFlashVerifyRespondTimerPost();

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[8])
	{
		m_u16TimerExpiredCnt[8] = RESEND_WAITING_START_CNT;
	}

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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	UINT16 j;

	//for test
	printf("Flash update: verify recv1,m_u16CurrentBlockNum=%d,m_u16BlockNumOfSection=%d\n", m_u16CurrentBlockNum, m_u16BlockNumOfSection);


	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;
	/*
	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/

	//如果进度不合法则放弃此次接收到的应答信号
	if (PROGRESS_IN_FLASH_PROG_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//校验成功,升级成功,反馈给后台TBD
	if (VERIFY_OK == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_PROG_ENA_OK;

			//本次SECTION所有BLOCK编程完成
			if (m_u16CurrentBlockNum >= (m_u16BlockNumOfSection - 1))
			{
				//准备去读下一个section
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_PROGRAM_ENABLE;
				//for test
				printf("Flash update: verify recv11\n");

			}

			//还有BLOCK需传输,
			else
			{
				//当前BLOCK号加1
				m_u16CurrentBlockNum++;
				//BLOCK地址加1024
				m_uBlockAddress = m_uBlockAddress + 1024;
				//状态机回到传输下一个BLOCK头
				m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
				//for test
				printf("Flash update: verify recv22\n");
			}

			//关闭定时器
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

	//校验失败,需要重新擦除FLASH
	else
	{
		//状态机回到准备下发擦除命令状态
		//m_pHostModuleItc->u16FlashupdateStatus = STATUS_API_OK;
		//--20100408:直接退出--
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;
		//升级进度标志复位
		m_u16ProgramPorcess = PROGRESS_IN_RESET;

		//返回给后台失败信息
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
BlockHeadXmitFcb-----下传BLOCK数据

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockHeadXmitFcb(VOID)
{
	UCHAR ucErrCode = CAN_MSG_HANDLE_OK;
	UINT16 u16BlockLen;
	UINT32 u32BlockAddr;

	//for test
	printf("Flash update: Send head--m_u16CurrentBlockNum=%d,m_u16BlockNumOfSection=%d\n", m_u16CurrentBlockNum, m_u16BlockNumOfSection);

	//取当前待传的BLOCK的长度和地址
	if (m_u16CurrentBlockNum >= (m_u16BlockNumOfSection - 1))
	{
		u16BlockLen = m_u16RemainWordOfSection;
		//总的帧数
		m_u16FrameNumOneBlock = m_u16FrameNumOfRemainBlock;
		//最后一帧字的个数
		m_u16LastFrameWordNumOneBlock = m_u16LastFrameWordNumOfRemainBlock;

		//本次BLOCK的校验和
		//m_u32CheckSumOneBlock = CheckSum(u16BlockLen, m_uBlockAddress, m_ucSectionBuff);
		m_u32CheckSumOneBlock
			= CheckSum(u16BlockLen, m_uBlockAddress, (m_ucSectionBuff + m_u16CurrentBlockNum * BLOCK_SIZE * 2));
	}

	else
	{
		//1024字
		u16BlockLen = BLOCK_SIZE;
		//一个完整的BLOCK需要分的帧数
		m_u16FrameNumOneBlock = FRAME_ALL_NUM_ONE_BLOCK;
		//最后一帧字的个数
		m_u16LastFrameWordNumOneBlock = LAST_FRAME_WORD_NUM_ONE_BLOCK;

		//本次BLOCK的校验和
		m_u32CheckSumOneBlock
			= CheckSum(u16BlockLen, m_uBlockAddress, (m_ucSectionBuff + m_u16CurrentBlockNum * BLOCK_SIZE * 2));
	}

	//当前帧号
	m_u16CurrentFrameNum = 0;

	//BLOCK地址
	u32BlockAddr = m_uBlockAddress;


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;
	//传送6字节信息头
	m_XmitMsg.u16Length = 6;

	//for test
	printf("Flash update: Send head--u16BlockLen=%d,u32BlockAddr=%d\n", u16BlockLen, u32BlockAddr);

	//将指针m_XmitMsg.pData也指向将要发送的数据
	//数据来源取自后台下传的DATA TBD
	*((UINT16 *)(m_XmitMsg.pData + 0)) = u16BlockLen;
	//*((UINT32 *)(m_XmitMsg.pData + 2)) = u32BlockAddr;
	*((UINT16 *)(m_XmitMsg.pData + 2)) = (UINT16)(u32BlockAddr & 0x0ffff);
	*((UINT16 *)(m_XmitMsg.pData + 4)) = (UINT16)((u32BlockAddr >> 16) & 0x0ffff);

	WaitHeadRespondTimerPost();
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_WAITING;

	//for test
	printf("Flash update: Send one block head\n");
	printf("Flash update: Send head--m_u16FrameNumOneBlock=%d\n", m_u16FrameNumOneBlock);

	/*
	//表示结束
	if (0 == m_wBlockLen)
	{
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_END;
	}

	else
	{
	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_WAITING;
	//开始计算下发数据的累加和校验码TBD
	//m_u16BlockChecksum = m_wBlockLen+m_uBlockAddress;
	}

	*/

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[5])
	{
		m_u16TimerExpiredCnt[5] = RESEND_WAITING_START_CNT;
	}

	return ucErrCode;
}

/**********************************************************************
BlockHeadRecvFcb-----下传BLOCK数据应答

Parameters:

Return Value:
Precondition:
Postcondition:
**********************************************************************/
INT32 CAN_FlashupdateMsgHandle::BlockHeadRecvFcb(VOID)
{
	UINT16 u16RetrunStatus;
	UINT16 j;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);
	//for test
	printf("Flash update: Recv one block head respond\n");
	/*
	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	return 1;
	}
	*/
	//如果进度不合法则放弃此次接收到的应答信号
	if (PROGRESS_IN_PROG_ENA_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//允许传有效数据,准备发数据信息
	if (BLOCK_HEAD_OK == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_HEAD_OK;
			//			m_u16ResendCnt = BLOCK_RESEND_CNT;

			//关闭定时器
			WaitHeadRespondTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[5] = RESEND_WAITING_RESET_CNT;

			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_HEADRESPOND_OK;

			//for test
			printf("Flash update: Recv one block head respond=0\n");
		}
	}

	//文件传输结束
	else if (FILE_TRANS_END == u16RetrunStatus)
	{
		//如果DSP反馈升级成功信息,同时需与监控自身标志核对,
		//如果监控标志表示文件的确传完才认为结束并报成功
		//监控本身认为HEX文件还没传完,则报失败
		if (READ_HEX_END_FLAG_VALID != m_u16ReadHexFileEnd)
		{
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;
			//升级进度标志复位
			m_u16ProgramPorcess = PROGRESS_IN_RESET;

			//返回给后台失败信息
			for (j = 0; j<11; j++)
			{
				m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
			}

			for (j = 0; j<TIMER_CNT_LEN; j++)
			{
				m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
			}

			//直接退出
			return 1;
		}


		//升级完成了一个模块
		//反馈给后台状态信息
		//旁路
		if (0x0f == m_RecvMsg.ucSourceId)
		{

			m_pHostModuleItc->u16UpdateStatus[0] = STATUS_HOST_UPDATE_SUCCESFULL;

		}

		else if ((0x20 <= m_RecvMsg.ucSourceId) && (0x29 >= m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16UpdateStatus[m_RecvMsg.ucSourceId - 0x20 + 1] = STATUS_HOST_UPDATE_SUCCESFULL;
		}

		else if ((0x10 <= m_RecvMsg.ucSourceId) && (0x19 >= m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16UpdateStatus[m_RecvMsg.ucSourceId - 0x10 + 1] = STATUS_HOST_UPDATE_SUCCESFULL;
		}

		//do nothing
		else
		{

		}


		//是否所有的模块都升级完毕
		//if (0 == FlashupdateNodeGet())
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			//m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

			//升级进度标志复位
			m_u16ProgramPorcess = PROGRESS_IN_RESET;

			//反馈给后台状态信息
			//m_pHostModuleItc->u16UpdateStatus[m_u16UpdatingModuleNo] = STATUS_HOST_UPDATE_SUCCESFULL;

			//reset task
			FlashupdateTaskReset();

			//			m_u16ResendCnt = BLOCK_RESEND_CNT;

			//关闭定时器
			WaitHeadRespondTimerFlush();

			//reset expired cnt
			m_u16TimerExpiredCnt[5] = RESEND_WAITING_RESET_CNT;

		}
		/*
		//one by one 下一个节点
		else
		{
		//m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_SELCET_NODE;

		//反馈给后台状态信息
		m_pHostModuleItc->u16UpdateStatus[m_u16UpdatingModuleNo] = STATUS_HOST_UPDAT_ING;
		}

		m_u16ResendCnt = BLOCK_RESEND_CNT;

		//关闭定时器
		WaitHeadRespondTimerFlush();
		*/
		//for test
		printf("Flash update: Recv one block head respond=5\n");
	}

	//需重发TBD
	else
	{
		//关闭定时器
		WaitHeadRespondTimerFlush();

		//reset task
		FlashupdateTaskReset();

		//reset expired cnt
		m_u16TimerExpiredCnt[5] = RESEND_WAITING_RESET_CNT;

		ResendOneBlock();
	}

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
	UINT16 u16DataLen;

	//for test
	printf("Flash update: Send one block data1 \n");

	//取当前需要传输的数据
	UINT16 *pu16Ptr = (UINT16*)(m_ucSectionBuff + m_u16CurrentBlockNum * BLOCK_SIZE * 2);
	pu16Ptr = pu16Ptr + m_u16CurrentFrameNum * 3;


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;

	//数据发送不需要下面回应,如果当前状态机不在发送数据时
	//将此次数据丢弃
	if ((STATUS_BLOCK_HEAD_OK != m_pHostModuleItc->u16FlashupdateStatus)
		&& (STATUS_BLOCK_DATATRANS_WAITING != m_pHostModuleItc->u16FlashupdateStatus))
	{
		return ucErrCode;
	}


	//当前BLOCK的最后一帧
	if (m_u16CurrentFrameNum >= (m_u16FrameNumOneBlock - 1))
	{
		//字节数
		m_XmitMsg.u16Length = m_u16LastFrameWordNumOneBlock * 2;

		for (UINT16 i = 0; i<m_u16LastFrameWordNumOneBlock; i++)
		{
			*((UINT16 *)(m_XmitMsg.pData + i * 2)) = *(pu16Ptr + i);
		}

		//等待传下一个BLOCK
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_DATATRANS_END;
	}

	else
	{
		//字节数
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
BlockDataRecvFcb-----下传BLOCK数据应答

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
	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/
	//允许传有效数据,准备发数据信息
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

	//擦除失败TBD
	else
	{
		ResendOneBlock();
	}
}



/**********************************************************************
BlockChecksumXmitFcb-----下传BLOCK校验和数据

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


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;
	//传送6字节信息
	m_XmitMsg.u16Length = 4;

	//将指针m_XmitMsg.pData也指向将要发送的数据
	//数据来源取自后台下传的DATA TBD
	//定义一个指针
	*((UINT16 *)(m_XmitMsg.pData + 0)) = (UINT16)(m_u32CheckSumOneBlock & 0x0ffff);
	*((UINT16 *)(m_XmitMsg.pData + 2)) = (UINT16)((m_u32CheckSumOneBlock >> 16) & 0x0ffff);

	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_WAITING;

	//启动重发定时器
	WaitBlockChksumRespondTimerPost();

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[6])
	{
		m_u16TimerExpiredCnt[6] = RESEND_WAITING_START_CNT;
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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	//for test
	printf("Flash update: Recv one block checksum respond1\n");

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;
	/*
	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/

	//如果进度不合法则放弃此次接收到的应答信号
	if (PROGRESS_IN_HEADRESPOND_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//允许传有效数据,准备发数据信息
	if (CHECK_SUM_SUCCESFUL == u16RetrunStatus)
	{
		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_OK;
			m_u16ResendCnt = BLOCK_RESEND_CNT;

			//关闭重发定时器
			WaitBlockChksumRespondTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[6] = RESEND_WAITING_RESET_CNT;

			//升级进度标志
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

	//校验失败,需重发TBD
	else
	{
		//关闭重发定时器
		WaitBlockChksumRespondTimerFlush();

		//reset task
		FlashupdateTaskReset();

		//reset expired cnt
		m_u16TimerExpiredCnt[6] = RESEND_WAITING_RESET_CNT;

		//		m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_CHECKSUM_OK;

		//升级进度标志,重发允许接收BLOCK头应答
		m_u16ProgramPorcess = PROGRESS_IN_PROG_ENA_OK;

		ResendOneBlock();
	}
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


	//升级与协议层交互的信息
	m_XmitMsg.ucRsRq = RS_MSG;
	//传送6字节信息
	m_XmitMsg.u16Length = 0;

	//将指针m_XmitMsg.pData也指向将要发送的数据
	//数据来源取自后台下传的DATA TBD
	//定义一个指针


	m_pHostModuleItc->u16FlashupdateStatus = STATUS_BLOCK_PROGRAM_WAITING;

	//启动重发定时器
	WaitBlockProgramRespondTimerPost();

	//超时计数器处理
	if (RESEND_WAITING_RESET_CNT == m_u16TimerExpiredCnt[7])
	{
		m_u16TimerExpiredCnt[7] = RESEND_WAITING_START_CNT;
	}

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
	UINT16 u16RetrunStatus;
	CAN_XMIT_QUEUE_MSG_T TempMsg;

	UINT16 j;

	//for test
	printf("Flash update: Recv program status respond1\n");

	u16RetrunStatus = *(UINT16 *)(m_RecvMsg.pData);;
	/*
	//如果收到的反馈擦除应答信息不对退出,如何处理TBD
	if (m_u16UpdaingNodeAdd != m_RecvMsg.ucSourceId)
	{
	//return 1;
	ResendOneBlock();
	}
	*/

	//如果进度不合法则放弃此次接收到的应答信号
	if (PROGRESS_IN_BLOCKCHECK_OK != m_u16ProgramPorcess)
	{
		return 1;
	}

	//允许传有效数据,准备发数据信息
	if (BLOCK_RECV_DATA_OK == u16RetrunStatus)
	{

		if (0 == FlashupdateTaskHandle(m_RecvMsg.ucSourceId))
		{
			//下发获取FLASH校验信息
			TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
			TempMsg.ucServiceCode = VERIFY_SRVCODE;
			TempMsg.ucMsgClass = m_ucMsgClass;
			TempMsg.ucFrag = NONFRAG_MSG;
			TempMsg.ucRsRq = RS_MSG;
			TempMsg.ucSourceId = MAC_ID_MON;
			CAN_MSG_Xmite(&TempMsg);

			//DSP反馈本次BLOCK编程成功
			//准备接收FLASH接收校验状态
			m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_VERIFYING;

			//关闭重发定时器
			WaitBlockProgramRespondTimerFlush();

			//reset task
			FlashupdateTaskReset();

			//reset expired cnt
			m_u16TimerExpiredCnt[7] = RESEND_WAITING_RESET_CNT;

			//升级进度标志
			m_u16ProgramPorcess = PROGRESS_IN_FLASH_PROG_OK;

			//			m_u16ResendCnt = BLOCK_RESEND_CNT;
		}

		//wait othger node repond
		else
		{
			;//do nothing
		}

	}

	//本次BLOCK编程失败,需重发TBD
	else
	{
		//
		//ResendOneBlock();
		//--20100408:直接退出--
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

		//升级进度标志复位
		m_u16ProgramPorcess = PROGRESS_IN_RESET;

		//返回给后台失败信息
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
NewPeriodicWaitMsgGen-----定时等待到则将擦除命令入队

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
			//超时退出升级,并且报失败
			//如果启动了则累加,没启动也不重发
			if (m_u16TimerExpiredCnt[i])
			{
				m_u16TimerExpiredCnt[i]++;

				if (m_u16TimerExpiredCnt[i] > RESEND_WAITING_MAX_CNT)
				{
					//处理握手应答超时到
					if (0 == i)
					{
						//取得后台指定升级但握手无反应的模块，认为不在线
						UINT16 u16NoRespondModule = (m_pHostModuleItc->u16ModIdx ^ m_u16RespondModuleFlag);

						//旁路不响应j=0,j=1~10 module1~10
						//不响应反馈给后台表示指定的模块不在线
						for (j = 0; j <11; j++)
						{
							if (((u16NoRespondModule >> j) & 1) == 1)
							{
								m_pHostModuleItc->u16UpdateStatus[j] = STATUS_UPDATE_NODE_OFF_LINE;
							}

						}

						//取得可以升级的模块
						m_pHostModuleItc->u16ModIdx &= m_u16RespondModuleFlag;

						//所有后台指定的模块均不能升级，退出
						if (0 == m_pHostModuleItc->u16ModIdx)
						{
							m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

							//升级进度标志复位
							m_u16ProgramPorcess = PROGRESS_IN_RESET;

							//返回给后台失败信息
							for (j = 0; j<11; j++)
							{
								m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
							}

							for (j = 0; j<TIMER_CNT_LEN; j++)
							{
								m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
							}
						}

						//后台指定的模块存在允许升级，则将状态机转到下一个状态
						else
						{
							ResetFlsUpdateMoudele();
							FlashupdateNodeGet();

							//下发获取芯片解密信息
							TempMsg.u16DestinationId = m_u16UpdaingNodeAdd;
							TempMsg.ucServiceCode = CHIP_DECODE_SRVCODE;
							TempMsg.ucMsgClass = m_ucMsgClass;
							TempMsg.ucFrag = NONFRAG_MSG;
							TempMsg.ucRsRq = RS_MSG;
							TempMsg.ucSourceId = MAC_ID_MON;
							CAN_MSG_Xmite(&TempMsg);

							//等待解密信息反馈,启动等待延时TBD
							m_pHostModuleItc->u16FlashupdateStatus = STATUS_WAITING_CHIP_DECODE;

							//升级进度标志
							m_u16ProgramPorcess = PROGRESS_IN_HAND_OK;

							//关握手定时器
							WaitHandleTimerFlush();

							FlashupdateTaskReset();

							m_u16TimerExpiredCnt[0] = RESEND_WAITING_RESET_CNT;

						}



					}
					//其他超时
					else
					{
						m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

						//升级进度标志复位
						m_u16ProgramPorcess = PROGRESS_IN_RESET;

						//返回给后台失败信息
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

					//启动定时器
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
TimeConvert-----将

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
FlashupdateNodeGet-----获取要升级的节点

Parameters:

Return Value: =0表示升级完成,=1还有节点待升级
Precondition:
Postcondition:
**********************************************************************/
UCHAR CAN_FlashupdateMsgHandle::FlashupdateNodeGet(VOID)
{
	UCHAR i, ucRet, j;
	UCHAR ucUpdateCnt;

	ucUpdateCnt = 0;

	//默认为广播
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
				//反馈给后台状态信息

				//升级旁路
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

	//同时升级两个以上节点
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
SetRespondModuleFlag-----FLASH UPDATE 相应模块有握手应答，说明可升级

Parameters:

Return Value: N/A
Precondition:
Postcondition:
**********************************************************************/
VOID CAN_FlashupdateMsgHandle::SetRespondModuleFlag(UCHAR ucRecvAddr)
{

	//整流
	if ((ucRecvAddr >= 0x20) && (ucRecvAddr <= 0x29))
	{
		//		m_u16RespondModuleFlag |= ((ucRecvAddr-0x20)<<1);
		m_u16RespondModuleFlag |= utFlashFlagMaskObj[(ucRecvAddr - 0x20) + 1].u16FlashFlagMask;
	}

	//逆变
	else if ((ucRecvAddr >= 0x10) && (ucRecvAddr <= 0x19))
	{
		//		m_u16RespondModuleFlag |= ((ucRecvAddr-0x10)<<1);

		m_u16RespondModuleFlag |= utFlashFlagMaskObj[(ucRecvAddr - 0x10) + 1].u16FlashFlagMask;
	}

	//旁路
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

//-----------------------------------------------------
/////////////////////////////////////////////////////////////////////////
//      读取文件头


//-----------------------------------------------------




//读一个扇区数据
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

//BLOCK重发机制m_u16ResendCnt
/**********************************************************************
ResendOneBlock-----获取要升级的节点

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
		//状态机回到传输下一个BLOCK头
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_NEXT_BLOCK_HEAD;
		//for test
		printf("Flash update: Resend a block head,m_u16ResendCnt=%d\n", m_u16ResendCnt);
		m_u16ResendCnt--;

	}

	//重发超3次报错并退出升级TBD
	else
	{
		//状态机回到传输下一个BLOCK头
		m_pHostModuleItc->u16FlashupdateStatus = STATUS_FLASH_UPDATE_OVER;

		//升级进度标志复位
		m_u16ProgramPorcess = PROGRESS_IN_RESET;

		//返回给后台失败信息
		for (j = 0; j<11; j++)
		{
			m_pHostModuleItc->u16UpdateStatus[j] = STATUS_HOST_UPDATE_FAIL;
		}

		for (j = 0; j<TIMER_CNT_LEN; j++)
		{
			m_u16TimerExpiredCnt[j] = RESEND_WAITING_RESET_CNT;
		}

		//反馈给后台状态信息
		m_pHostModuleItc->u16UpdateStatus[m_u16UpdatingModuleNo] = STATUS_HOST_UPDATE_FAIL;

		//for test
		printf("Flash update: Resend overtime,m_u16ResendCnt=%d\n", m_u16ResendCnt);
	}
}

