#include "all_headers.h"
#include "hostsedit.h"

#pragma comment(lib, "Shlwapi.lib")

HostsEdit::HostsEdit() : szTitle_(L"added by Windscribe, do not modify.")
{
    wchar_t szPath[MAX_PATH];
    GetSystemDirectory(szPath, MAX_PATH);
    szSystemDir_ = szPath;
}

HostsEdit::~HostsEdit()
{
}

bool HostsEdit::removeWindscribeHosts()
{
    // remove hosts with strings "windscribe" from hosts file
    FILE *fileTmp = _wfopen(getTempHostsPath().c_str(), L"w");
    bool bSuccess = false;
    if (fileTmp)
    {
        FILE *file = _wfopen(getHostsPath().c_str(), L"a+");
        if (file)
        {
            std::vector<std::wstring> strs;
            wchar_t buf[10000];
            while (!feof(file))
            {
                fgetws(buf, 10000, file);
                if (StrStrI(buf, L"windscribe") == NULL)
                {
                    strs.push_back(buf);
                }
            }

            for (std::vector<std::wstring>::iterator it = strs.begin(); it != strs.end(); ++it)
            {
                std::wstring str = *it;
                // remove newline for last string
                if (it == (strs.end() - 1))
                {
                    if (it->size() > 0 && str[wcslen(it->c_str())-1] == L'\n')
                    {
                        str = it->substr(0, it->size()-1);
                    }
                }
                fputws(str.c_str(), fileTmp);
            }

            fclose(file);
            bSuccess = true;
        }
        fclose(fileTmp);
    }

    if (bSuccess)
    {
		return (MoveFileEx(getTempHostsPath().c_str(), getHostsPath().c_str(), MOVEFILE_REPLACE_EXISTING) != 0);
    }
	else
	{
		return false;
	}
}

bool HostsEdit::addHosts(std::wstring szHosts)
{
	FILE *file = _wfopen(getHostsPath().c_str(), L"a+");

	if (file)
	{
		//first read all strings from file to vector
		std::vector<std::wstring> strs;
		wchar_t buf[10000];
		while (!feof(file))
		{
			fgetws(buf, 10000, file);
			strs.push_back(buf);
		}

		std::vector<std::wstring> hosts = split(szHosts, L';');
		std::vector<std::wstring> added_hosts;
		for (std::vector<std::wstring>::iterator it = hosts.begin(); it != hosts.end(); ++it)
		{
			if (!stringInVector(strs, *it))
			{
				added_hosts.push_back(*it);
			}
		}

		if (added_hosts.size() > 0)
		{
			// write newline to end of file, if needed
			fseek(file, 1, SEEK_END);
			char c;
			fread(&c, 1, 1, file);

			if (c != '\n')
			{
                fseek(file, 0, SEEK_END);
				fputws(L"\n", file);
			}
		}
		for (std::vector<std::wstring>::iterator it = added_hosts.begin(); it != added_hosts.end(); ++it)
		{
			fputws(it->c_str(), file);
			fputws(L"   #", file);
			fputws(szTitle_.c_str(), file);
			if (it != added_hosts.end() - 1)
			{
				fputws(L"\n", file);
			}
		}
		fclose(file);
		return true;
	}
	else
	{
		return false;
	}
}

bool HostsEdit::removeHosts()
{
	FILE *fileTmp = _wfopen(getTempHostsPath().c_str(), L"w");
	bool bSuccess = false;
	if (fileTmp)
	{
		FILE *file = _wfopen(getHostsPath().c_str(), L"a+");
		if (file)
		{
			std::vector<std::wstring> strs;
			wchar_t buf[10000];
			while (!feof(file))
			{
				fgetws(buf, 10000, file);
				if (wcsstr(buf, szTitle_.c_str()) == NULL)
				{
					strs.push_back(buf);
				}
			}

			for (std::vector<std::wstring>::iterator it = strs.begin(); it != strs.end(); ++it)
			{
				std::wstring str = *it;
				// remove newline for last string
				if (it == (strs.end() - 1))
				{
					if (it->size() > 0 && str[wcslen(it->c_str()) - 1] == L'\n')
					{
						str = it->substr(0, it->size() - 1);
					}
				}
				fputws(str.c_str(), fileTmp);
			}

			fclose(file);
			bSuccess = true;
		}
		fclose(fileTmp);
	}

	if (bSuccess)
	{
		return (MoveFileEx(getTempHostsPath().c_str(), getHostsPath().c_str(), MOVEFILE_REPLACE_EXISTING) != 0);
	}
	else
	{
		return false;
	}
}

std::wstring HostsEdit::getHostsPath()
{
    wchar_t szPath[MAX_PATH];
    PathCombine(szPath, szSystemDir_.c_str(), L"drivers\\etc\\hosts");
    return szPath;
}

std::wstring HostsEdit::getTempHostsPath()
{
    wchar_t szPath[MAX_PATH];
    PathCombine(szPath, szSystemDir_.c_str(), L"drivers\\etc\\hosts.tmp");
    return szPath;
}

bool HostsEdit::stringInVector(std::vector<std::wstring> &vec, std::wstring &str)
{
	for (std::vector<std::wstring>::iterator it = vec.begin(); it != vec.end(); ++it)
	{
		if (wcsstr(it->c_str(), str.c_str()) != NULL)
		{
			return true;
		}
	}
	return false;
}

std::vector<std::wstring> &HostsEdit::split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems)
{
	std::wstringstream ss(s);
	std::wstring item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::wstring> HostsEdit::split(const std::wstring &s, wchar_t delim)
{
	std::vector<std::wstring> elems;
	split(s, delim, elems);
	return elems;
}