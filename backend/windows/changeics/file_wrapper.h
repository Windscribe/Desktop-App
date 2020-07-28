#pragma once
#include <stdio.h>
#include <vector>
#include <rpc.h>
#include <NetCon.h>

typedef struct
{
	GUID guid;
	VARIANT_BOOL bSharingEnabled;
	SHARINGCONNECTIONTYPE typeOfSharing;
} SHARING_OPTIONS;

class FileWrapper
{
public:
	FileWrapper();
	~FileWrapper();

	bool open(wchar_t *filePath, wchar_t *mode);

	bool write(std::vector<SHARING_OPTIONS> &v);
	bool read(std::vector<SHARING_OPTIONS> &v);

private:
	FILE *file_;
};

