
// flash_apiDlg.h : ͷ�ļ�
//

#pragma once


// Cflash_apiDlg �Ի���
class Cflash_apiDlg : public CDialogEx
{
// ����
public:
	Cflash_apiDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FLASH_API_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl FlashUpdateProgress;
	afx_msg void OnBnClickedStart();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedStartCanAnalyse();
	afx_msg void OnBnClickedCalaulator();
};
