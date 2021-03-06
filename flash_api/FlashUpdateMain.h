#pragma once

#include "CAN_FlashupdateMsgHandle.h"
#include "ControlCAN.h"
#include "afxwin.h"
#include "Blob.h"
#include <memory>

// FlashUpdateInv 对话框

//#define RegisterDDX_Control(MODULE_ID)\
//DDX_Control(pDX, IDC_INV_MODILE##MODULE_ID, FlashUpdateProgressCtrl[MODULE_ID])

//class	Blob;
class	CAN_FlashupdateMsgHandle;
class FlashUpdateMain : public CDialogEx
{

	DECLARE_DYNAMIC(FlashUpdateMain)
private:
	HICON m_hIcon;

	afx_msg int OnBnClickedOpenCanDevice();
	afx_msg void OnBnClickedCloseCanDevice();
	void TryCloseDevice();
	int FlashUpdateProgressSetPos();
	//int Hex_file_resolve();
	int FlashUpdateProgress[16];
	bool SendFlashUpdateOrder();

	UINT16 time_out_cnt;
	int ParameterRefresh();
	VCI_CAN_OBJ msg_init;
	std::shared_ptr<CAN_FlashupdateMsgHandle> uniflash;
	std::shared_ptr<Blob> Solver;

	int FlashUpdateGetNode();
	int device_type = 4;	// CANalyst-II
	int device_ind = 0;		// first device
	int can_ind = 0;		// CAN channel 0
	BYTE service_code;
	BYTE tx_fu_data[4];

	CProgressCtrl FlashUpdateProgressCtrl[16];
	
	void  ShowButton();
	// get dialog size
	CRect myRect;
	bool ChangeSize(UINT id, int x, int y);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	
	DECLARE_MESSAGE_MAP()

public:
	FlashUpdateMain(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~FlashUpdateMain();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedStartFlashUpdate();
	
	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FLASH_UPDATE_MAIN };
#endif
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg void OnBnClickedRecordSaved();
	afx_msg void OnClose();
	afx_msg void OnBnClickedClearMsg();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnBnClickedBootLoader();
	afx_msg void OnBnClickedStartMsg();
	afx_msg void OnBnClickedStopMsg();
	afx_msg void OnCbnSelchangeModuleSelect();

	afx_msg void OnBnClickedMfccolorbutton1();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	
};

