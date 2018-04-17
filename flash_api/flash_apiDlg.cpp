
// flash_apiDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "flash_api.h"
#include "flash_apiDlg.h"
#include "afxdialogex.h"
#include "ControlCAN.h"
#include "CanAnalyse.h"
#include "FlashUpdateInv.h"
//#pragma comment(lib, "ControlCAN.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cflash_apiDlg �Ի���



Cflash_apiDlg::Cflash_apiDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_FLASH_API_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cflash_apiDlg::DoDataExchange(CDataExchange* pDX)
{


	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, FlashUpdateProgress);
}

BEGIN_MESSAGE_MAP(Cflash_apiDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &Cflash_apiDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_ADD, &Cflash_apiDlg::OnBnClickedAdd)
	ON_EN_CHANGE(IDC_LEFT, &Cflash_apiDlg::OnEnChangeLeft)
	ON_EN_CHANGE(IDC_RESULT, &Cflash_apiDlg::OnEnChangeResult)
	ON_BN_CLICKED(ID_START, &Cflash_apiDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_CLOSE, &Cflash_apiDlg::OnBnClickedClose)
	ON_BN_CLICKED(IDC_ADD, &Cflash_apiDlg::OnBnClickedAdd)
	ON_BN_CLICKED(ID_OPENDEVICE, &Cflash_apiDlg::OnBnClickedOpendevice)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_START_CAN_ANALYSE, &Cflash_apiDlg::OnBnClickedStartCanAnalyse)
END_MESSAGE_MAP()


// Cflash_apiDlg ��Ϣ�������

BOOL Cflash_apiDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void Cflash_apiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Cflash_apiDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR Cflash_apiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void Cflash_apiDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	MessageBox(_T("let's go!"));
	//CDialogEx::OnOK();
}





void Cflash_apiDlg::OnBnClickedAdd()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	int left = GetDlgItemInt(IDC_LEFT);
	int right = GetDlgItemInt(IDC_RIGHT);
	SetDlgItemInt(IDC_RESULT,left+right);
	char hello[] = "come on!";
	SetDlgItemText(IDC_GOOGLE, _T("come on!"));
	//SetDlgItemText(IDC_RESULT, _T("hello"));/
	//char hello[] = "hello, world";
	//MessageBox(_T("let's go!"));
	//while (1);
}


void Cflash_apiDlg::OnEnChangeLeft()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void Cflash_apiDlg::OnEnChangeResult()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}




void Cflash_apiDlg::OnEnChangeGoogle()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}

/****************************************************************************
pInitConfig->AccCode AccCode ��ӦSJA1000 �е��ĸ��Ĵ���ACR0��ACR1��ACR2��
ACR3�����и��ֽڶ�ӦACR0�����ֽڶ�ӦACR3��AccMask
��ӦSJA1000 �е��ĸ��Ĵ���AMR0��AMR1��AMR2��AMR3��
���и��ֽڶ�ӦAMR0�����ֽڶ�ӦAMR3�����뿴���˵����
pInitConfig->AccMask
pInitConfig->Reserve
d
����
pInitConfig->Filter �˲���ʽ��1 ��ʾ���˲���0 ��ʾ˫�˲�
pInitConfig->Timing0 �����ʶ�ʱ��0�����VCI_INIT_CONFIG
pInitConfig->Timing1 �����ʶ�ʱ��1�����VCI_INIT_CONFIG
pInitConfig->Mode ģʽ��0 ��ʾ����ģʽ��1 ��ʾֻ��ģʽ

CAN		������		��ʱ��0		��ʱ��1
5Kbps		0xBF		0xFF
10Kbps		0x31		0x1C
20Kbps		0x18		0x1C
40Kbps		0x87		0xFF
50Kbps		0x09		0x1C
80Kbps		0x83		0Xff
100Kbps		0x04		0x1C
125Kbps		0x03		0x1C
200Kbps		0x81		0xFA
250Kbps		0x01		0x1C
400Kbps		0x80		0xFA
500Kbps		0x00		0x1C
666Kbps		0x80		0xB6
800Kbps		0x00		0x16
1000Kbps	0x00		0x14
**************************************************************************/
void Cflash_apiDlg::OnBnClickedStart()
{
	OnOK();
	FlashUpdateInv	FlashUpdateInv;;
	FlashUpdateInv.DoModal();

}


void Cflash_apiDlg::OnBnClickedClose()
{
	FlashUpdateProgress.SetPos(75);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	int device_type = 4;	// CANalyst-II
	int device_ind = 0;		// first device
	int can_ind = 0;		// CAN channel 0
	DWORD	test;
	test = VCI_CloseDevice(device_type, device_ind);
	if (test == STATUS_ERR) {
		MessageBox(_T("�ر��豸ʧ�ܣ�"), _T("����"), MB_OK | MB_ICONQUESTION);
	}
	else {
		SetWindowText(_T("Close Device Success!"));
	}
	SetDlgItemText(IDC_OUT_TXT, _T("go! go!! go!!!"));

}


void Cflash_apiDlg::OnBnClickedButton2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	int left = GetDlgItemInt(IDC_LEFT);
	int right = GetDlgItemInt(IDC_RIGHT);
	SetDlgItemInt(IDC_RESULT, left + right);
	char hello[] = "come on!";
	SetDlgItemText(IDC_GOOGLE, _T("come on!"));
	//SetDlgItemText(IDC_RESULT, _T("hello"));/
	//char hello[] = "hello, world";
	//MessageBox(_T("let's go!"));
	//while (1);

	CString	b;
	b += "\nad�뿪ʱ�䷢�������˿��Ϳ����ǲ������ӹ������ɭ�ļ��ʲ����˿�ʱ��ֲ���˹�����";
	b += "\nad�뿪ʱ�䷢�������˿����ǲ�������ǲ������ӹ������ɭ�ļ��ʲ����˿�ʱ��ֲ���˹�����";
	SetDlgItemText(IDC_OUT_TXT, b);
}



void Cflash_apiDlg::OnBnClickedOpendevice()
{
	//Login login_in;
	//login_in.DoModal();
	FlashUpdateProgress.SetPos(25);
	int device_type = 4;	// CANalyst-II
	int device_ind = 0;		// first device
	int can_ind = 0;		// CAN channel 0
	VCI_INIT_CONFIG	vic;
	DWORD	test;
	vic.AccCode = 0x00000000;
	vic.AccMask = 0xFFFFFFFF;
	vic.Filter = 0;
	vic.Mode = 0;
	vic.Timing0 = 0x03;
	vic.Timing1 = 0x1C;
	vic.Reserved = 0;
	test = VCI_OpenDevice(device_type, device_ind, can_ind);

	if (test != STATUS_OK) {
		MessageBox(_T("Open Device Failed!!!"), _T("Caution"), MB_OK | MB_ICONQUESTION);
		return;
	}

	test = VCI_InitCAN(device_type, device_ind, can_ind, &vic);
	if (test == STATUS_ERR) {
		VCI_CloseDevice(device_type, device_ind);
		MessageBox(_T("Init Device Failed��"), _T("Caution"), MB_OK | MB_ICONQUESTION);
		return;
	}

	test = VCI_StartCAN(device_type, device_ind, can_ind);
	if (test == STATUS_ERR) {
		VCI_CloseDevice(device_type, device_ind);
		MessageBox(_T("Start Device Failed��"), _T("Caution"), MB_OK | MB_ICONQUESTION);
		return;
	}
	else {
		SetWindowText(_T("Open Device Success��"));
	}

}


void Cflash_apiDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	//AnimateWindow(1000, AW_CENTER);
}


void Cflash_apiDlg::OnBnClickedStartCanAnalyse()
{
	OnOK();
	CanAnalyse	OpenCanDevice;
	OpenCanDevice.DoModal();
}
