#include "stdafx.h"
#include "Blob.h"
#include "flash_api.h"
#include "FlashUpdateMain.h"
#include "afxdialogex.h"
#include "flash_apiDlg.h"
#include "ControlCAN.h"
#include "CAN_FLASHupdateMsgHandle.h"
#include <afxdb.h>
#include <stdexcept>
Blob::Blob(CString file_path) :outfilepath(file_path)
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


Blob::~Blob()
{

}

int Blob::BootLoaderFileResolve() {

	CFile file;
	CFileException ex;
	CString tarfile_path;
	if (!file.Open(_T("./target_file_boot.hex"), CFile::modeRead | CFile::shareDenyWrite, &ex))
		throw std::runtime_error("Please check flashupdate file!!!");
	
	DWORD file_length = (DWORD)file.GetLength();
	if (file.GetLength() > 1000000L) 
		throw std::runtime_error("file too large!!!");

	char *p = new char[file_length];
	file.Read(p, file_length);
	file.Close();


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
	return TRUE;
}



int Blob::Hex_file_resolve()
{
	system("del target_file.hex");

	//WinExec("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out", SW_NORMAL);
	//system("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out");
	//system("del 28377D_INV.hex");
	//system("cmd");
	CFile	file;
	CFileException ex;
	
	CString target_file_path;
	target_file_path = _T("hex2000.exe --memwidth=16 --romwidth=16 --intel -o ./target_file.hex ");

	target_file_path += outfilepath;
	//char abcd[] = T2A(target_file_path.GetBuffer(target_file_path.GetLength()));
	USES_CONVERSION;
	//MessageBox(target_file_path, _T("警告"), MB_OK | MB_ICONQUESTION);
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



/*
void FlashUpdateMain::OnBnClickedStartFlashUpdate()
{

#define RECORD_TYPE_OFFSET	3
#define	DATA_OFFSET	4
#define	HIGH_16BIT_ADDRESS_RECORD_TYPE	0x04
#define	DATA_DEFINICATION	0x00
#define DATA_END_RECORD_TYPE	0x01
	if (IDNO == AfxMessageBox(_T("Are you sure Flashupdate?"), MB_YESNO))return;
	//WinExec("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out", SW_NORMAL);
	//system("hex2000.exe --memwidth=16 --romwidth=16 --intel -o G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.hex  G:/CCSV7workspace/28377_UPS/28377D_INV/FLASH_RUN/28377D_INV.out");
	//system("del 28377D_INV.hex");

	CFile	file;
	CFile	wfile;
	CFileException ex;
	CString filepath;
	CString	file_read;
	UINT16	check_every_block_size = 0;
	char	check_low16_address = 0;

	GetDlgItemText(IDC_MFCEDITBROWSE1, filepath);
	if (!file.Open(filepath, CFile::modeRead | CFile::shareDenyWrite, &ex)) {

		MessageBox(_T("请检查待升级文件！"), _T("警告"), MB_OK | MB_ICONQUESTION);
		return;
	}
	DWORD file_length = (DWORD)file.GetLength();
	char *p = new char[file_length];
	char my_file[500000];
	UINT16 OddorEven = 0;
	filepath = "Cell50KInvLib.hex";
	file.Read(p, file_length);
	file.Close();

	DWORD	char_num = 0;
	for (UINT32 k = 0; k < file_length; ++k) {
		if ((p[k] == ':') || (p[k] == '\r') || (p[k] == '\n')) {
			my_file[char_num] = p[k];
			char_num++;
			if (p[k] == ':') {
				OddorEven = k % 2;
			}
		}
		else {
			if (OddorEven) {
				if ((p[k] == 'A') || (p[k] == 'B') || (p[k] == 'C') ||
					(p[k] == 'D') || (p[k] == 'E') || (p[k] == 'F'))
				{
					if (k % 2) {
						my_file[char_num] |= (p[k] - 55);
						char_num++;
					}
					else {
						my_file[char_num] = (p[k] - 55) << 4;

					}
				}
				else {
					if (k % 2) {
						my_file[char_num] |= (p[k] - 48);
						char_num++;
					}
					else {
						my_file[char_num] = (p[k] - 48) << 4;

					}
				}
			}
			else if (OddorEven == 0) {
				if ((p[k] == 'A') || (p[k] == 'B') || (p[k] == 'C') ||
					(p[k] == 'D') || (p[k] == 'E') || (p[k] == 'F'))
				{
					if (k % 2) {
						my_file[char_num] = (p[k] - 55) << 4;

					}
					else {
						my_file[char_num] |= (p[k] - 55);
						char_num++;
					}
				}
				else {
					if (k % 2) {
						my_file[char_num] = (p[k] - 48) << 4;

					}
					else {
						my_file[char_num] |= (p[k] - 48);
						char_num++;
					}
				}
			}
		}
	}

	while (transfer_data.next_data_head < char_num) {
		for (DWORD i = transfer_data.next_data_head; i < char_num; ++i) {

			if (my_file[i] == ':') {
				transfer_data.data_head = i + 1;
			}
			if ((my_file[i] == '\r') && (my_file[i + 1] == '\n') && (my_file[i + 2] == ':')) {
				transfer_data.data_trail = i - 1;
				transfer_data.next_data_head = i + 2;
				transfer_data.data_count = transfer_data.data_trail - transfer_data.data_head + 1;
				break;
			}

		}
		// Record Type( 00, 04, 01)
		if (my_file[transfer_data.data_head + RECORD_TYPE_OFFSET] == HIGH_16BIT_ADDRESS_RECORD_TYPE) {

			BlockAddress[BlockCount][1] = (((UINT16)(my_file[transfer_data.data_head + 4])) << 8) |
				(((UINT16)(my_file[transfer_data.data_head + 5])) & 0xFF);
			continue;
		}
		else if (my_file[transfer_data.data_head + RECORD_TYPE_OFFSET] == DATA_END_RECORD_TYPE) {

			break;

		}
		else {

			check_every_block_size = EveryBlockDataNum[BlockCount] + (((UINT16)(my_file[transfer_data.data_head])) & 0xFF);
			if (EveryBlockDataNum[0] == 0) {

				BlockAddress[BlockCount][0] = (((UINT16)(my_file[transfer_data.data_head + 1])) << 8) |
					(((UINT16)(my_file[transfer_data.data_head + 2])) & 0xFF);
			}
			if (check_every_block_size > 2048) {
				++BlockCount;
				BlockAddress[BlockCount][0] = (((UINT16)(my_file[transfer_data.data_head + 1])) << 8) |
					(((UINT16)(my_file[transfer_data.data_head + 2])) & 0xFF);
			}
			else if (check_low16_address != my_file[transfer_data.data_head + 2]) {

				++BlockCount;
				BlockAddress[BlockCount][0] = (((UINT16)(my_file[transfer_data.data_head + 1])) << 8) |
					(((UINT16)(my_file[transfer_data.data_head + 2])) & 0xFF)
			}
			check_low16_address = (my_file[transfer_data.data_head] / 2) + my_file[transfer_data.data_head + 2];
			for (UINT32 j = transfer_data.data_head + DATA_OFFSET; j < transfer_data.data_trail; ++j) {

				if (j % 2) {

					BlockData[BlockCount][EveryBlockDataNum[BlockCount]] = (((UINT16)(my_file[j])) << 8);

				}
				else {

					BlockData[BlockCount][EveryBlockDataNum[BlockCount]] |= (((UINT16)(my_file[j])) & 0x00FF);
					++EveryBlockDataNum[BlockCount];
				}
			}

		}


	}

	delete[]p;
}
*/


/*
UINT16 recore_type_04_line[10];
for (UINT16 i = 0; i < 10; ++i) {

recore_type_04_line[i] = 0;
}
UINT16 recore_type_04 = 0;
for (DWORD i = 0; i < line_count; ++i) {

if (every_line_recore_type[i] == 4) {

recore_type_04_line[recore_type_04] = i;
++recore_type_04;
}
}

UINT32 now_address = 0;
UINT32 backup = 0;
UINT32	high32_address = 0;
for (UINT16 i = 0; i < recore_type_04; ++i) {

for (DWORD j = 0; j < line_count; ++j) {
if (every_line_recore_type[j] == 4) {

high32_address = ((((UINT32)every_line_data[recore_type_04_line[i++]][0]) << 8) & 0x00FF0000L);


}
else {

every_line_address[now_address] = high32_address + every_line_address[j];
++now_address;
}
}
}
*/

