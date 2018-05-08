// This class transfer .out file to HEX file,
// read hex file using CFile Class, and transfer ASCII to UINT16

#pragma once
#include "Blob.h"
class OutFileResolve :
	public Blob
{
public:
	explicit OutFileResolve(CString out_file_path);
	virtual ~OutFileResolve();

public:
	/*UINT16	BlockCount;
	UINT16	EveryBlockDataNum[500];
	UINT16	BlockData[500][1024];
	UINT32	BlockCheckSum[500];
	UINT32	BlockAddress[500];
	*/
	virtual BOOL FileResolve();

private:
	CString OutFilePath;
	char*	ReadFile(CString file_path, DWORD *file_length);
};

