
// flash_api.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Cflash_apiApp: 
// �йش����ʵ�֣������ flash_api.cpp
//

class Cflash_apiApp : public CWinApp
{
public:
	Cflash_apiApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cflash_apiApp theApp;