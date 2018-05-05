#pragma once
#include "Blob.h"


class BootFileResolve : public Blob
{

public:
	explicit BootFileResolve(CString boot_file_path);
	virtual ~BootFileResolve();

	//char	BootLoaderFile[MAX_BOOT_FILE_LENGTH];
	//DWORD	BootFileCount;
	virtual BOOL FileResolve();

private:
	char*	ReadFile(CString boot_file_path, DWORD *file_length);
	CString BootFilePath;

};

