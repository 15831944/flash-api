#include "stdafx.h"
#include "Blob.h"
#include "flash_api.h"
#include "FlashUpdateMain.h"
#include "afxdialogex.h"
#include "flash_apiDlg.h"
#include "ControlCAN.h"
#include "CAN_FLASHupdateMsgHandle.h"
#include "BootFileResolve.h"
#include <afxdb.h>
#include <stdexcept>
#include <memory>



BootFileResolve::BootFileResolve(CString boot_file_path):
	BootFilePath(boot_file_path)
{
	for (int i = 0; i < MAX_BOOT_FILE_LENGTH; ++i) {

		BootLoaderFile[i] = 0;
	}

}

BootFileResolve::~BootFileResolve()
{
}



BOOL BootFileResolve::FileResolve() {

	/*CFile file;
	CFileException ex;
	CString tarfile_path;
	if (!file.Open(_T("./target_file_boot.hex"), CFile::modeRead | CFile::shareDenyWrite, &ex))
		throw std::runtime_error("Please check flashupdate file!!!");

	DWORD file_length = (DWORD)file.GetLength();
	if (file.GetLength() > 1000000L)
		throw std::runtime_error("file too large!!!");

	char *p = new char[file_length];
	file.Read(p, file_length);
	file.Close();*/

	DWORD file_length;
	char* p = ReadFile(BootFilePath, &file_length);

	char *resolve_hex_file = new char[file_length];

	for (DWORD i = 0; i < file_length; ++i) {

		resolve_hex_file[i] = 0;
	}

	DWORD file_count = 0;
	// Resolve Hex File, // Except ':', '/r', '/n',
	// Place in resolve_hex_file[LINE_COUNT][EVERY_LINE_CHAR_NUMBER]
	for (DWORD i = 0; i < file_length; ++i) {

		if ((p[i] == 'A') || (p[i] == 'B') || (p[i] == 'C') ||
			(p[i] == 'D') || (p[i] == 'E') || (p[i] == 'F') ||
			(p[i] == '1') || (p[i] == '2') || (p[i] == '3') ||
			(p[i] == '4') || (p[i] == '5') || (p[i] == '6') ||
			(p[i] == '7') || (p[i] == '8') || (p[i] == '9') ||
			(p[i] == '0'))
		{

			resolve_hex_file[file_count] = p[i];
			file_count++;
		}
	}
	BootFileCount = 0;
	for (DWORD i = 0; i < file_count; ++i) {

		if ((resolve_hex_file[i] == 'A') || (resolve_hex_file[i] == 'B') || (resolve_hex_file[i] == 'C') ||
			(resolve_hex_file[i] == 'D') || (resolve_hex_file[i] == 'E') || (resolve_hex_file[i] == 'F')) {

			if (i % 2) {

				BootLoaderFile[i / 2] |= resolve_hex_file[i] - 55;
			}
			else {

				BootLoaderFile[i / 2] = ((resolve_hex_file[i] - 55) << 4) & 0xF0;
			}
		}
		else {

			if (i % 2) {

				BootLoaderFile[i / 2] |= resolve_hex_file[i] - 48;
			}
			else {

				BootLoaderFile[i / 2] = ((resolve_hex_file[i] - 48) << 4) & 0xF0;
			}
		}

	}
	BootFileCount = file_count / 2;

	delete resolve_hex_file;
	delete p;
	return TRUE;
}


char* BootFileResolve::ReadFile(CString boot_file_path, DWORD *file_length) {

	system("del boot_file.hex");

	//WinExec("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out", SW_NORMAL);
	//system("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out");
	//system("del 28377D_INV.hex");
	//system("cmd");
	CFile	file;
	CFileException ex;

	CString target_file_path;
	target_file_path = _T("hex2000.exe --ascii -o ./boot_file.hex -boot -gpio8 ");
	target_file_path += boot_file_path;
	//char abcd[] = T2A(target_file_path.GetBuffer(target_file_path.GetLength()));
	USES_CONVERSION;
	//MessageBox(target_file_path, _T("¾¯¸æ"), MB_OK | MB_ICONQUESTION);
	//CHAR *abcd = T2A(target_file_path.GetBuffer(0));
	//T2A(m_strCMD.GetBuffer(m_strCMD.GetLength()))
	WinExec(T2A(target_file_path.GetBuffer(target_file_path.GetLength())), SW_HIDE);

	if (IDNO == AfxMessageBox(_T("Are you sure boot loader?"), MB_YESNO)) {

		system("del boot_file.hex");
		throw std::runtime_error("boot loader failed!");
	}
	if (!file.Open(_T("./boot_file.hex"), CFile::modeRead | CFile::shareDenyWrite, &ex))
		throw std::runtime_error("Please check boot loader file!!!");

	*file_length = (DWORD)file.GetLength();
	if (file.GetLength() > 1000000L)
		throw std::runtime_error("file too large!!!");

	char *p = new char[*file_length];
	file.Read(p, *file_length);
	file.Close();
	system("del boot_file.hex");

	return p;
}

