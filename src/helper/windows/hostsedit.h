#pragma once

#include <string>
#include <vector>

class HostsEdit
{
public:
    static HostsEdit &instance()
    {
        static HostsEdit he;
        return he;
    }

    bool removeWindscribeHosts();
    bool addHosts(std::wstring szHosts);
    bool removeHosts();

private:
    std::wstring szTitle_;
    std::wstring szSystemDir_;

    HostsEdit();
    virtual ~HostsEdit();
    std::wstring getHostsPath();
    std::wstring getTempHostsPath();
    bool stringInVector(std::vector<std::wstring> &vec, std::wstring &str);
    std::vector<std::wstring> &split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems);
    std::vector<std::wstring> split(const std::wstring &s, wchar_t delim);
};
