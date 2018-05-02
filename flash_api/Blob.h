#pragma once
class Blob
{
public:
	UINT16	BlockCount;
	UINT16	EveryBlockDataNum[500];
	UINT16	BlockData[500][1024];
	UINT32	BlockCheckSum[500];
	UINT32	BlockAddress[500];



	char	BootLoaderFile[1000000];
	DWORD	BootFileCount;
	explicit Blob(CString out_file_path);
	~Blob();
	BOOL Hex_file_resolve() ;
	BOOL BootLoaderFileResolve() ;

private:
	CString OutFilePath;
	//CString BootFilePath;
	char*	ReadFile(CString file_path, DWORD *file_length);
};

