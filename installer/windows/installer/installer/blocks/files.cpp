#include "files.h"
#include "../../../Utils/applicationinfo.h"
#include "../../../Utils/Path.h"
#include "../../../Utils/utils.h"

using namespace std;

Files::Files(const wstring &installPath, double weight) : IInstallBlock(weight, L"Files"), archive_(NULL),
	installPath_(installPath), state_(0)
{
}

Files::~Files()
{
	SAFE_DELETE(archive_);
}

int Files::executeStep()
{
	if (state_ == 0)
	{
		SAFE_DELETE(archive_);
		archive_ = new Archive(L"Windscribe");

		SRes res = archive_->fileList(fileList_);

		if (res != SZ_OK)
		{
			lastError_ = L"Can't get files list from archive.";
			return -1;
		}

		bool win10_or_greater = IsWindows10OrGreater();
		bool isX64 = isWin64();

		fileList_.remove_if(FilterFiles(isX64, win10_or_greater));
		fillPathList();

		archive_->calcTotal(fileList_, pathList_);
		curFileInd_ = 0;
		state_++;
		return 0;
	}
	else
	{
		SRes res = archive_->extractionFile(curFileInd_);
		if (res != SZ_OK)
		{
			archive_->finish();
			lastError_ = L"Can't extract file.";
			return -1;
		}
		else 
		{
			int progress = ((double)curFileInd_ / (double)archive_->getNumFiles()) * 100.0;

			if (curFileInd_ >= (archive_->getNumFiles() - 1))
			{
				archive_->finish();
				return 100;
			}
			else
			{
curFileInd_++;
			}
			return progress;
		}
	}

	return 100;
}


bool Files::isWin64()
{
	bool is_64_bit = true;

	if (FALSE == GetSystemWow64DirectoryW(nullptr, 0u))
	{
		const DWORD last_error = GetLastError();
		if (ERROR_CALL_NOT_IMPLEMENTED == last_error)
		{
			is_64_bit = false;
		}
	}

	return is_64_bit;
}

void Files::fillPathList()
{
	pathList_.clear();
	for (auto it = fileList_.cbegin(); it != fileList_.cend(); it++)
	{
		wstring destination;

		if ((*it).find(L"tap6") != std::string::npos)
		{
			destination = installPath_ + L"\\tap\\" + getFileName(*it);
		}
		else if ((*it).find(L"wintun") != std::string::npos)
		{
			destination = installPath_ + L"\\wintun\\" + getFileName(*it);
		}
		else if ((*it).find(L"splittunnel") != std::string::npos)
		{
			destination = installPath_ + L"\\splittunnel\\" + getFileName(*it);
		}
		else if (((*it).find(L"x64\\") != std::string::npos) || ((*it).find(L"x64/") != std::string::npos) || ((*it).find(L"x32\\") != std::string::npos) || ((*it).find(L"x32/") != std::string::npos))
		{
			destination = installPath_ + L"\\" + getFileName(*it);
		}
		else
		{
			destination = installPath_ + L"\\" + *it;
		}

		pathList_.push_back(Path::PathExtractDir(destination));
	}
}


wstring Files::getFileName(const wstring &s)
{
	wstring str;

	size_t i = s.rfind('\\', s.length());

	if (i == string::npos)
	{
		i = s.rfind('/', s.length());
	}


	if (i != string::npos)
	{
		str = s.substr(i + 1, s.length() - i);
	}

	return str;
}

FilterFiles::FilterFiles(bool isX64, bool win10_or_greater) :
	isX64_(isX64), win10_or_greater_(win10_or_greater)
{
}

bool FilterFiles::operator()(const std::wstring &value) const
{
	bool file_name = false;
	size_t i = value.rfind('.', value.length());

	if (i != string::npos)
	{
		size_t len = value.length();
		if ((i == (len - 4)) || (i == (len - 5)))
		{
			file_name = true;
		}
	}

	// return true if need remove
	bool ret = false;
	if (!file_name)
	{
		ret = true;
	}
	else if (value.find(L"splittunnel") != std::string::npos || value.find(L"wintun") != std::string::npos || value.find(L"tap6") != std::string::npos)
	{
		ret = ((value.find(L"i386") != std::string::npos) && (isX64_)) || 
			  ((value.find(L"amd64") != std::string::npos) && (!isX64_));

		ret |= ((value.find(L"win7") != std::string::npos) && (win10_or_greater_)) ||
			((value.find(L"win10") != std::string::npos) && (!win10_or_greater_));
	}
	
	else if (value.find(L"x32/") != std::string::npos)
	{
		ret = isX64_;
	}
	else if (value.find(L"x64/") != std::string::npos)
	{
		ret = !isX64_;
	}

	return ret;
}