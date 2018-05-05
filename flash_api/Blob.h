#pragma once
#define MAX_BOOT_FILE_LENGTH 1000000
class Blob
{
public:
	explicit Blob();
	virtual ~Blob();
	virtual BOOL FileResolve() = 0;

	UINT16	BlockCount;
	UINT16	EveryBlockDataNum[500];
	UINT16	BlockData[500][1024];
	UINT32	BlockCheckSum[500];
	UINT32	BlockAddress[500];

	char	BootLoaderFile[MAX_BOOT_FILE_LENGTH];
	DWORD	BootFileCount;
};

