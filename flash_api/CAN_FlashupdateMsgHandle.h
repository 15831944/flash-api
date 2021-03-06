// This is a class, process some message, 
// which is commmunicate with TI C2000 MCU: TMS320F2808, TMS320F28377D etc;
// This Class have two function: Online FlashUpdate, BootLoader;
// 


#pragma once
//--------------------------------------------------------------
//head files
#include <stdio.h>
#include <string.h>
#include "stdafx.h"
#include "flash_api.h"
#include "FlashUpdateMain.h"
#include "afxdialogex.h"
#include "flash_apiDlg.h"
#include "ControlCAN.h"
#include "Blob.h"

#define MAX_ERROR_MSG	100
#define MESSAGE_NUM 500
#define CHECK_RX_MESSAGE_NUM() if (msg_num == 0)return 0;\
							msg_num = (msg_num > MESSAGE_NUM)? MESSAGE_NUM:msg_num;


#define MESSAGE_FILLTER(SERVICECODE) ((RXMessage[i].PackedMsg.b10MsgClass == CAN_RESERVED_CLASS) &&	\
									(RXMessage[i].PackedMsg.b6DestinationMacId == MAC_ID_MON)  &&	\
									(RXMessage[i].PackedMsg.b7ServiceCode == SERVICECODE) && \
									(RXMessage[i].PackedMsg.b6SourceMacId >= NodeOffset) && \
									(RXMessage[i].PackedMsg.b6SourceMacId < (NodeOffset+0x10)))

#define	TX_MESSAGE_FUNCTION(MSG_NUMBER, SERVICE_CODE, DLC)	\
	TXMessage[MSG_NUMBER].PackedMsg.RemoteFlag = 0;			\
	TXMessage[MSG_NUMBER].PackedMsg.ExternFlag = 0;				\
	TXMessage[MSG_NUMBER].PackedMsg.b6DestinationMacId = m_u16UpdaingNodeAdd;\
	TXMessage[MSG_NUMBER].PackedMsg.b7ServiceCode = SERVICE_CODE;\
	TXMessage[MSG_NUMBER].PackedMsg.b10MsgClass = m_ucMsgClass;\
	TXMessage[MSG_NUMBER].PackedMsg.b1Fragment = NONFRAG_MSG;\
	TXMessage[MSG_NUMBER].PackedMsg.b1RsRq = RQ_MSG;\
	TXMessage[MSG_NUMBER].PackedMsg.b6SourceMacId = MAC_ID_MON;\
	TXMessage[MSG_NUMBER].PackedMsg.DataLen = DLC;\


//-----------------------------------------------------------------------------
//Macro definition
//sevice code denfine
//握手
#define HANDS_COMM_SRVCODE 0x01
//芯片解密
#define CHIP_DECODE_SRVCODE 0x02
//API版本信息
#define API_VERSION_SRVCODE 0x03
//擦除扇区
#define ERASE_SECTOR_SRVCODE 0x04
//编程开始指令
#define PROGRAM_SRVCODE 0x05
//传输BLOCK文件头信息
#define BLOCK_HEAD_SRVCODE 0x06
//相应BLOCK传输有效数据
#define BLOCK_DATA_SRVCODE 0x07
//传输相应BLOCK校验和--累加和
#define BLOCK_CHECKSUM_SRVCODE 0x08
//传输对应BLOCK的program状态
#define BLOCK_PROMG_STATUS_SRVCODE 0x09
//FLASH 校验
#define VERIFY_SRVCODE 0x0A

//-------------------------------------
//升级进度标志相关宏定义
//进度复位状态
#define	PROGRESS_IN_RESET		0xff
//升级开始
#define	PROGRESS_IN_START		0
//握手OK
#define 	PROGRESS_IN_HAND_OK		1
//解密OK
#define	PROGRESS_IN_DECODE_OK	2
//API版本OK
#define	PROGRESS_IN_APIVERSION_OK	3
//擦除扇区OK
#define	PROGRESS_IN_ERASE_OK		4
//编程允许OK
#define	PROGRESS_IN_PROG_ENA_OK	5
//BLOCK头信息应答OK
#define	PROGRESS_IN_HEADRESPOND_OK	6
//BLOCK校验和应答OK
#define	PROGRESS_IN_BLOCKCHECK_OK	7
//FLASH 编程OK
#define	PROGRESS_IN_FLASH_PROG_OK	8
//FLASH 编程校验OK
#define	PROGRESS_IN_FLASH_VERIFY_OK	9

//------------------------------------


//状态信息宏定义
//握手,下发
#define HAND_COMM_QUERY 0x62
//握手上传
#define HAND_OK_RESPOND 0xaa

//解密成功
#define CHIP_DECODE_SUCCESS 0x00
#define CHIP_DECODE_FAIL	   10

//API版本匹配问题
#define API_VESION_OK	0x6c
#define API_VERSION_FAIL 14




//扇区擦除相关
//扇区选择
#define ERASE_SECTOR_ALL 6
#define ERASE_SECTOR_A 1
#define ERASE_SECTOR_B 2
#define ERASE_SECTOR_C 3
#define ERASE_SECTOR_D 4
#define ERASE_SECTOR_BCD 5

//扇区擦除返回码定义
//擦除成功
#define ERASE_SUCCESFULL 0
//未指定扇区
#define ERASE_NO_SPECIFIED_SECTOR 20
//清0失败
#define ERASE_PRECONDITION_FAIL 21
//扇区擦除失败
#define ERASE_FAIL 22
//紧致失败
#define ERASE_COMPACT_FAIL 23
//预紧致失败
#define ERASE_PRECOMPACT_FAIL 24

//PROGRAM
//编程允许
#define PROGRAM_ENABLE 0x6e
//因扇区未擦除不允许编程
#define PROGRAM_DIABLE 0x70

//VERIFY
//校验成功
#define VERIFY_OK	0
//校验失败
#define VERIFY_FAIL 10

//数据传输
//BLOCK传输头正确
#define BLOCK_HEAD_OK	0
//BLOCK头正确接收并文件传输结束
#define FILE_TRANS_END 5
//BLOCK SIZE >1024
#define BLOCK_SIZE_OVERLOW 10
//BLOCK ADD OVERLOW
#define BLOCK_ADD_OVERLOW 20
//BLOCK head and add are overlow
#define BLOCK_SEZE_ADD_OVERLOW	30

//one block check sum
#define CHECK_SUM_SUCCESFUL	0x82
#define CHECK_SUM_FAIL			0x83


//dsp 反馈接收到数据
#define PROGRAM_STATUS_SUCCESS 0


//-----20100118:增加处理共同升级整流、逆变的处理变量类型定义
//目标升级使能与否
#define TARGET_UPDATE_ENABLE 1
#define TARGET_UPDATE_DISABLE 0

//升级过程中本次任务处理完成与否
#define TASK_HANDLED 1
#define TASK_HANDLE_NON 0


//数据域长度
#define CAN_DATA_FIELD_SIZE 8

//数据长度
#define CAN_NON_FRAG_DATA_LEN 6
#define CAN_FRAG_DATA_LEN		500


#define FRAG_MSG 1
#define NONFRAG_MSG 0

#define RS_MSG 1
#define RQ_MSG 0


//CAN信息类别枚举
enum CAN_MSG_CLASS_ENUM
{
	CAN_RESERVED_CLASS = 0,
	CAN_CFG_MSG_CLASS = 1,
	CAN_CTRL_MSG_CLASS = 2,
	CAN_ALRM_MSG_CLASS = 3,
	CAN_STATE_MSG_CLASS = 4,
	CAN_ANALOG_MSG_CLASS = 5,
	CAN_MSG_CLASS_EOL
};


enum CAN_MSG_HANGLE_ERR_ENUM
{
	CAN_MSG_HANDLE_OK = 0,
	CAN_MSG_HANDLE_INVALID_MAC_ID,
	CAN_MSG_HANDLE_INVALID_BATT_GRP,
	CAN_MSG_HANDLE_INVALID_OBJ,
	CAN_MSG_HANDLE_INVALID_MSG_CLASS,
	CAN_MSG_HANDLE_INVALID_SRVC_COD,


	CAN_MSG_HANDLE_EOL
};
// CAN ID allocated
enum _CAN_MAC_ID_ENUM
{
	MAC_ID_RSVDX00 = 0,
	MAC_ID_MON = 1,
	MAC_ID_RSVDX02,
	MAC_ID_RSVDX03,
	MAC_ID_RSVDX04,
	MAC_ID_RSVDX05,
	MAC_ID_RSVDX06,
	MAC_ID_RSVDX07,
	MAC_ID_RSVDX08,
	MAC_ID_RSVDX09,
	MAC_ID_RSVDX0A,
	MAC_ID_RSVDX0B,
	MAC_ID_RSVDX0C,
	MAC_ID_RSVDX0D,
	MAC_ID_RSVDX0E,
	MAC_ID_BYP = 0x0F,
	MAC_ID_MOD1_INV = 0x10,
	MAC_ID_MOD2_INV = 0x11,
	MAC_ID_MOD3_INV = 0x12,
	MAC_ID_MOD4_INV = 0x13,
	MAC_ID_MOD5_INV = 0x14,
	MAC_ID_MOD6_INV = 0x15,
	MAC_ID_MOD7_INV = 0x16,
	MAC_ID_MOD8_INV = 0x17,
	MAC_ID_MOD9_INV = 0x18,
	MAC_ID_MOD10_INV = 0x19,
	MAC_ID_MOD11_INV = 0x1A,
	MAC_ID_MOD12_INV = 0x1B,
	MAC_ID_MOD13_INV = 0x1C,
	MAC_ID_MOD14_INV = 0x1D,
	MAC_ID_MOD15_INV = 0x1E,
	MAC_ID_MOD16_INV = 0x1F,
	MAC_ID_MOD1_REC = 0x20,
	MAC_ID_MOD2_REC = 0x21,
	MAC_ID_MOD3_REC = 0x22,
	MAC_ID_MOD4_REC = 0x23,
	MAC_ID_MOD5_REC = 0x24,
	MAC_ID_MOD6_REC = 0x25,
	MAC_ID_MOD7_REC = 0x26,
	MAC_ID_MOD8_REC = 0x27,
	MAC_ID_MOD9_REC = 0x28,
	MAC_ID_MOD10_REC = 0x29,
	MAC_ID_MOD11_REC = 0x2A,
	MAC_ID_MOD12_REC = 0x2B,
	MAC_ID_MOD13_REC = 0x2C,
	MAC_ID_MOD14_REC = 0x2D,
	MAC_ID_MOD15_REC = 0x2E,
	MAC_ID_MOD16_REC = 0x2F,
	MAC_ID_EOL
};


typedef enum {

	//初始无效状态
	FLASH_UPDATE_INVALID = 0,
	//开始选择节点升级
	FLASH_UPDATE_START,
	//发送握手信号等待握手信号状态
	SEND_MSG_WAITING_HANDS_RESPOND,
	//发送芯片解密命令等待芯片解密应答信号
	SEND_MSG_WAITING_CHIP_DECODE_RESPOND,
	//发送命令等待API版本确认信息
	SEND_MSG_WAITING_API_VERSION_OK,
	//发送擦除命令
	SEND_MSG_FLASH_ERASE,
	//等待擦除完毕
	WAITING_MSG_ERASE_END,
	//请求编程许可等待编程允许
	SEND_MSG_PROGRAM_PERMIT_WAITING_RESPOND,
	//传输BLOCK头
	SEND_MSG_BLOCK_HEAD,
	//等待传输BLOCK头结束标志
	WAITING_BLOCK_HEAD_TRANSFER_OK,
	//传输BLOCK数据
	SEND_BLOCK_DATA,
	//等待传输BLOCK数据完成标志
	WAITING_MSG__BLOCK_DATATRANS_END,
	//传输CHECKSUM
	SEND_MSG_BLOCK_CHECKSUM,
	//等待CHECKSUM校验成功标志
	WAITING_MSG_BLOCK_CHECKSUM_OK,
	//下发编程命令,
	SEND_ORDER_PROGRAM,
	//等待DSP回传编程状态
	WAITING_MSG_PROGRAM_OK,
	//发送FLASH 校验命令,
	SEND_ORDER_FLASH_VERIFY,
	//等待FLASH 校验完毕消息
	WAITING_FLASH_VERIFY_OK,
	//判断是否还有BLOCK需传输, 有则传输BLOCK头,无则向DSP发送重启命令
	SEND_NEXT_BLOCK_OR_SEND_DSP_RESTART_MSG_WAITING,
	//等待DSP FLASHUPDATE成功标志
	WAITING_FLAG_FLASHUPDATE_COMPLETED,
	FLASH_UPDATE_SUCCEED,
	//FLASH UPDATE 失败
	FLASH_UPDATE_OVER
}_FLASHUPDATE_STATUS;


typedef struct _HOST_MODULE_ITC_T1
{

	//是否需要升级相应kernel
	//=0x95表示需升级kernel,其他不升级kernel
	UINT16 u16FlashUpdateKernelFlag;

	//Flash update 状态机
	_FLASHUPDATE_STATUS u16FlashupdateStatus;

	//.....
}_HOST_MODULE_ITC_T;

typedef struct _ERROR_MESSAGE1{

	BYTE ReceiveDone;
	DWORD ErrorCnt;
	_FLASHUPDATE_STATUS ErrorStateSaved;

}_ERROR_MESSAGE;


typedef union CAN_PACKED_PROTOCOL_STRUCT
{
	VCI_CAN_OBJ Frame;
	struct
	{
		UINT16	b6SourceMacId : 6;
		UINT16	b10MsgClass : 4;
		UINT16	NotUsed1 : 6;
		UINT16	NotUsed2;
		UINT	TimeStamp;
		BYTE	TimeFlag;
		BYTE	SendType;
		BYTE	RemoteFlag;//是否是远程帧
		BYTE	ExternFlag;//是否是扩展帧
		BYTE	DataLen;				//Data field all len,bytes
		BYTE	b6DestinationMacId : 6;
		BYTE	b1NotUsed : 1;
		BYTE	b1Fragment : 1;
		BYTE	b7ServiceCode : 7;
		BYTE	b1RsRq : 1;
		BYTE	MsgData[6];
		BYTE	Reserved[3];
	}PackedMsg;
}CAN_PACKED_PROTOCOL_U;


class CAN_FlashupdateMsgHandle
{
public:
	CAN_FlashupdateMsgHandle(const Blob &solver);
	virtual ~CAN_FlashupdateMsgHandle(VOID);

	VOID GetFlashUpdateRoutine(VOID);
	//---------------------------------
	INT32 FlashUpdateInvalid(VOID);
	INT32 ParameterRefresh(VOID);
	INT32 HandCommProcess(VOID);
	INT32 ChipDecodeProcess(VOID);
	INT32 VerifyApiVersion(VOID);
	INT32 EraseSectorOrderXmit(VOID);
	INT32 EraseSectorStatusRecv(VOID);
	INT32 ProgramPermissionGet(VOID);
	INT32 BlockHeadXmit(VOID);
	INT32 BlockHeadRecv(VOID);
	INT32 BlockDataXmit(VOID);
	INT32 BlockDataRecv(VOID);
	INT32 BlockCheckSumXmit(VOID);
	INT32 BlockCheckSumRecv(VOID);
	INT32 BlockProgOrderXmit(VOID);
	INT32 BlockProgOrderRecv(VOID);
	INT32 VerifyXmit(VOID);
	INT32 VerifyRecv(VOID);
	INT32 SendNextBlock_DspRestart(VOID);
	INT32 FlashUpdateComplete(VOID);
	INT32 FlashUpdateSucceed(VOID);
	INT32 FlashUpdateOver(VOID);
	//当前正在升级的节点地址
	UINT16 m_u16UpdaingNodeAdd;


	VOID GetBootLoaderRoutine(VOID);
private:

	UCHAR m_ucMsgClass;


	int DeviceType = 4;	// CANalyst-II
	int DeviceInd = 0;		// first device
	int CanInd = 0;		// CAN channel 0
	CAN_PACKED_PROTOCOL_U	*TXMessage;
	CAN_PACKED_PROTOCOL_U	*RXMessage;



	void	MsgErrorProcess(_FLASHUPDATE_STATUS flash_update_state,
							BOOL IsNot);
	UINT64	MsgReceivedDoneFlagSave;
	UINT64  MsgErrorSave;
	UINT64	NodeSelect;


	UINT16  BlockCount;
	const Blob    &Solver;
	
	UINT32 BootLoaderCount;
public:
	BYTE * BootMsgPtr;
	// 0x00, 0x10, 0x20
	UINT16 NodeOffset;
	// 1 - 16
	UINT64 Module_number;
	// 0x01: moninter	0x0F:BYP	0x10 - 0x1F: REC    0x20 - 0x2F  INV
	UINT16	FlashUpdateProgress[0x3F];	

	_ERROR_MESSAGE  FlashUpdateErrorMsg[0x3F];
	_HOST_MODULE_ITC_T *m_pHostModuleItc;

	_ERROR_MESSAGE  FlashUpdateErrorMsgBak[0x3F];
	_HOST_MODULE_ITC_T m_pHostModuleItcBak;

};

