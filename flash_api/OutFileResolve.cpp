#include "stdafx.h"
#include "Blob.h"
#include "flash_api.h"
#include "FlashUpdateMain.h"
#include "afxdialogex.h"
#include "flash_apiDlg.h"
#include "ControlCAN.h"
#include "CAN_FLASHupdateMsgHandle.h"
#include "OutFileResolve.h"
#include <afxdb.h>
#include <stdexcept>
#include <memory>

OutFileResolve::OutFileResolve(CString out_file_path) :
	OutFilePath(out_file_path)
{
	BlockCount = 0;
	for (UINT16 i = 0; i < 500; ++i) {
		EveryBlockDataNum[i] = 0;
		BlockCheckSum[i] = 0;
		BlockAddress[i] = 0;
		for (UINT16 j = 0; j < 1024; ++j) {

			BlockData[i][j] = 0;
		}
	}
}


OutFileResolve::~OutFileResolve()
{
}


BOOL OutFileResolve::FileResolve()
{
	/*
	system("del target_file.hex");

	//WinExec("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out", SW_NORMAL);
	//system("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out");
	//system("del 28377D_INV.hex");
	//system("cmd");
	CFile	file;
	CFileException ex;

	CString target_file_path;
	target_file_path = _T("hex2000.exe --memwidth=16 --romwidth=16 --intel -o ./target_file.hex ");

	target_file_path += OutFilePath;
	//char abcd[] = T2A(target_file_path.GetBuffer(target_file_path.GetLength()));
	USES_CONVERSION;
	//MessageBox(target_file_path, _T("少御"), MB_OK | MB_ICONQUESTION);
	//CHAR *abcd = T2A(target_file_path.GetBuffer(0));
	//T2A(m_strCMD.GetBuffer(m_strCMD.GetLength()))
	WinExec(T2A(target_file_path.GetBuffer(target_file_path.GetLength())), SW_HIDE);

	if (IDNO == AfxMessageBox(_T("Are you sure Flashupdate?"), MB_YESNO)) {

	system("del target_file.hex");
	throw std::runtime_error("flash update failed!");
	}
	if (!file.Open(_T("./target_file.hex"), CFile::modeRead | CFile::shareDenyWrite, &ex))
	throw std::runtime_error("Please check flashupdate file!!!");

	DWORD file_length = (DWORD)file.GetLength();
	if (file.GetLength() > 1000000L)
	throw std::runtime_error("file too large!!!");

	char *p = new char[file_length];
	file.Read(p, file_length);
	file.Close();
	system("del target_file.hex");
	*/
	DWORD file_length;
	char* p = ReadFile(OutFilePath, &file_length);

	// Initialization
#define LINE_COUNT 5000
#define EVERY_LINE_CHAR_NUMBER	74

	char resolve_hex_file[LINE_COUNT][EVERY_LINE_CHAR_NUMBER];
	char char_to_number[LINE_COUNT][EVERY_LINE_CHAR_NUMBER / 2];
	for (DWORD i = 0; i < LINE_COUNT; ++i) {
		for (DWORD j = 0; j < EVERY_LINE_CHAR_NUMBER; ++j) {

			resolve_hex_file[i][j] = 0;
		}
		for (DWORD k = 0; k < EVERY_LINE_CHAR_NUMBER / 2; ++k) {

			char_to_number[i][k] = 0;
		}
	}
	char every_line_count[LINE_COUNT];
	for (DWORD i = 0; i < LINE_COUNT; ++i) {

		every_line_count[i] = 0;
	}





	DWORD	line_count = 0;

	// Resolve Hex File, // Except ':', '/r', '/n',
	// Place in resolve_hex_file[LINE_COUNT][EVERY_LINE_CHAR_NUMBER]
	for (DWORD i = 0; i < file_length; ++i) {

		if ((p[i] == ':') || (p[i] == '\r') || (p[i] == '\n')) {}
		else {

			resolve_hex_file[line_count][every_line_count[line_count]] = p[i];
			every_line_count[line_count]++;
			if (every_line_count[line_count] > EVERY_LINE_CHAR_NUMBER)
				throw std::runtime_error("file too many columns!!!");
		}
		if (p[i] == '\n') {

			line_count++;
			if (every_line_count[line_count] > LINE_COUNT)
				throw std::runtime_error("file too Rows!!!");
		}
	}





	// ASCII to char number
	for (DWORD i = 0; i < line_count; ++i) {

		for (char j = 0; j < every_line_count[i]; ++j) {

			if ((resolve_hex_file[i][j] == 'A') || (resolve_hex_file[i][j] == 'B') || (resolve_hex_file[i][j] == 'C')
				|| (resolve_hex_file[i][j] == 'D') || (resolve_hex_file[i][j] == 'E') || (resolve_hex_file[i][j] == 'F')) {

				if (j % 2) {

					char_to_number[i][j / 2] |= resolve_hex_file[i][j] - 55;
				}
				else {

					char_to_number[i][j / 2] = ((resolve_hex_file[i][j] - 55) << 4) & 0xF0;
				}
			}
			else {

				if (j % 2) {

					char_to_number[i][j / 2] |= resolve_hex_file[i][j] - 48;
				}
				else {

					char_to_number[i][j / 2] = ((resolve_hex_file[i][j] - 48) << 4) & 0xF0;
				}
			}
		}
	}






	// line count from ASCII to char number
	for (DWORD i = 0; i < line_count; ++i) {

		every_line_count[i] = every_line_count[i] / 2;
	}
	// char number to UINT16, address resolve to 32 bit
	UINT16	every_line_data[LINE_COUNT][16];
	UINT32	every_line_address[LINE_COUNT];
	UINT16	every_line_data_count[LINE_COUNT];
	UINT16	every_line_recore_type[LINE_COUNT];
	for (DWORD i = 0; i < LINE_COUNT; ++i) {

		for (int j = 0; j < 16; ++j) {

			every_line_data[i][j] = 0;
		}
		every_line_address[i] = 0x00000000;
		every_line_data_count[i] = 0;
		every_line_recore_type[i] = 0;
	}
	for (DWORD i = 0; i < line_count; ++i) {

		every_line_recore_type[i] = (((UINT16)char_to_number[i][3]) & 0x00FF);
		every_line_data_count[i] = char_to_number[i][0];
		every_line_address[i] = ((((UINT32)char_to_number[i][1]) << 8) & 0x0000FF00L) +
			(((UINT32)char_to_number[i][2]) & 0x000000FFL);

		for (char j = 4; j < every_line_count[i] - 1; ++j) {

			if (j % 2) {

				every_line_data[i][j / 2 - 2] |= (((UINT16)char_to_number[i][j]) & 0x00FF);
			}
			else {

				every_line_data[i][j / 2 - 2] = ((((UINT16)char_to_number[i][j]) << 8) & 0xFF00);
			}
		}
	}








	// line count from char to UINT16
	for (DWORD i = 0; i < line_count; ++i) {

		every_line_count[i] = (every_line_count[i] - 5) / 2;
	}
	UINT16	new_every_line_data[LINE_COUNT][16];
	UINT32	new_every_line_address[LINE_COUNT];
	UINT16	new_every_line_data_count[LINE_COUNT];
	UINT32	new_line_count = 0;
	UINT32	high32address = 0;
	for (DWORD i = 0; i < LINE_COUNT; ++i) {

		new_every_line_address[i] = 0;
		new_every_line_data_count[i] = 0;
		for (int j = 0; j < 16; ++j) {

			new_every_line_data[i][j] = 0;
		}
	}






	// remove recore type, adress from 16bit become 32 bit address, so line number decrease
	for (DWORD i = 0; i < line_count; i++) {
		if (every_line_recore_type[i] == 4) {

			high32address = (((UINT32)every_line_data[i][0]) << 16) & 0xFFFF0000;
		}
		else if (every_line_recore_type[i] == 1) {
			break;
		}
		else {

			new_every_line_address[new_line_count] = high32address + every_line_address[i];
			new_every_line_data_count[new_line_count] = every_line_data_count[i] / 2;
			for (UINT16 j = 0; j < 16; j++) {

				new_every_line_data[new_line_count][j] = every_line_data[i][j];
			}
			++new_line_count;
		}
	}



	BlockCount = 0;
	// place message to block; waiting fir transmit, finally, resolved hex file successfully
	UINT16 BlockCountBak = 0;
	for (DWORD i = 0; i < new_line_count; ++i) {
		if (i != 0) {
			if (new_every_line_address[i] !=
				(new_every_line_address[i - 1] + new_every_line_data_count[i - 1]))
			{
				BlockCount++;

			}
		}
		if ((EveryBlockDataNum[BlockCount] + new_every_line_data_count[i]) > 1024) {

			BlockCount++;
		}
		if (BlockCount == BlockCountBak) {

			BlockAddress[BlockCount] = new_every_line_address[i];
			BlockCountBak++;
		}
		for (UINT16 j = 0; j < new_every_line_data_count[i]; ++j) {

			BlockData[BlockCount][EveryBlockDataNum[BlockCount]] = new_every_line_data[i][j];
			EveryBlockDataNum[BlockCount]++;
		}
	}
	BlockCount++;






	// calcluate check sum
	for (DWORD i = 0; i < BlockCount; ++i) {

		BlockCheckSum[i] += BlockAddress[i];
		BlockCheckSum[i] += EveryBlockDataNum[i];
		for (UINT16 j = 0; j < EveryBlockDataNum[i]; ++j) {

			BlockCheckSum[i] += BlockData[i][j];
		}
	}



	delete[]p;
	return TRUE;
}

char* OutFileResolve::ReadFile(CString file_path, DWORD *file_length) {

	system("del target_file.hex");

	//WinExec("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out", SW_NORMAL);
	//system("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out");
	//system("del 28377D_INV.hex");
	//system("cmd");
	CFile	file;
	CFileException ex;

	CString target_file_path;
	target_file_path = _T("hex2000.exe --memwidth=16 --romwidth=16 --intel -o ./target_file.hex ");

	target_file_path += file_path;
	//char abcd[] = T2A(target_file_path.GetBuffer(target_file_path.GetLength()));
	USES_CONVERSION;
	//MessageBox(target_file_path, _T("少御"), MB_OK | MB_ICONQUESTION);
	//CHAR *abcd = T2A(target_file_path.GetBuffer(0));
	//T2A(m_strCMD.GetBuffer(m_strCMD.GetLength()))
	WinExec(T2A(target_file_path.GetBuffer(target_file_path.GetLength())), SW_HIDE);

	if (IDNO == AfxMessageBox(_T("Are you sure Flashupdate?"), MB_YESNO)) {

		system("del target_file.hex");
		throw std::runtime_error("flash update failed!");
	}
	if (!file.Open(_T("./target_file.hex"), CFile::modeRead | CFile::shareDenyWrite, &ex))
		throw std::runtime_error("Please check flashupdate file!!!");

	*file_length = (DWORD)file.GetLength();
	if (file.GetLength() > 1000000L)
		throw std::runtime_error("file too large!!!");

	char *p = new char[*file_length];
	file.Read(p, *file_length);
	file.Close();
	system("del target_file.hex");

	return p;
}

