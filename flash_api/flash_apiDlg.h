
// flash_apiDlg.h : 头文件
//

#pragma once


// Cflash_apiDlg 对话框
class Cflash_apiDlg : public CDialogEx
{
// 构造
public:
	Cflash_apiDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FLASH_API_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:

	CProgressCtrl FlashUpdateProgress;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnEnChangeLeft();
	afx_msg void OnEnChangeResult();
	afx_msg void OnEnChangeGoogle();
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedClose();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedOpendevice();
	afx_msg void OnCbnSelchangemoduleselect();
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnNMCustomdrawProgress1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeDspSelect();
	afx_msg void OnEnChangeMfceditbrowse1();
	afx_msg void OnStnClickedOutTxt();
	afx_msg void OnEnChangeOutTxt();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnEnChangeId();
	afx_msg void OnEnChangeDlc();
	afx_msg void OnEnChangeData();
};
