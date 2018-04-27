#pragma once
class Blob
{
public:
	UINT16	BlockCount;
	UINT16	EveryBlockDataNum[500];
	UINT16	BlockData[500][1024];
	UINT32	BlockCheckSum[500];
	UINT32	BlockAddress[500];

	CString outfilepath;

	explicit Blob(CString file_path);
	~Blob();
	int Hex_file_resolve();
};

