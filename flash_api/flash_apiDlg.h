
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
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnEnChangeLeft();
	afx_msg void OnEnChangeResult();
	afx_msg void OnEnChangego();
	afx_msg void OnEnChangeGoogle();
	afx_msg void OnBnClickedStart();
};
