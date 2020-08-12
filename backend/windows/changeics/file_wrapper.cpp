#include "file_wrapper.h"


FileWrapper::FileWrapper() : file_(NULL)
{
}


FileWrapper::~FileWrapper()
{
	if (file_)
	{
		fclose(file_);
	}
}

bool FileWrapper::open(wchar_t *filePath, wchar_t *mode)
{
    if (!file_)
	    file_ = _wfopen(filePath, mode);
	return file_ != NULL;
}

bool FileWrapper::write(std::vector<SHARING_OPTIONS> &v)
{
	size_t sz = v.size();
	if (!fwrite(&sz, sizeof(sz), 1, file_))
	{
		return false;
	}

	for (std::vector<SHARING_OPTIONS>::iterator it = v.begin(); it != v.end(); ++it)
	{
		SHARING_OPTIONS opts = *it;
		if (!fwrite(&opts, sizeof(opts), 1, file_))
		{
			return false;
		}
	}

	return true;
}

bool FileWrapper::read(std::vector<SHARING_OPTIONS> &v)
{
	v.clear();
	size_t sz;
	if (!fread(&sz, sizeof(sz), 1, file_))
	{
		return false;
	}
	v.reserve(sz);
	for (size_t i = 0; i < sz; ++i)
	{
		SHARING_OPTIONS opts;
		if (!fread(&opts, sizeof(opts), 1, file_))
		{
			return false;
		}
		v.push_back(opts);
	}

	return true;
}
