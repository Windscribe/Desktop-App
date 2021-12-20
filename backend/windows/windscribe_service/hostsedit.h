#ifndef HOSTSEDIT_H
#define HOSTSEDIT_H

class HostsEdit
{
public:
    HostsEdit();
    virtual ~HostsEdit();

    bool removeWindscribeHosts();
	bool addHosts(std::wstring szHosts);
	bool removeHosts();

private:
	std::wstring szTitle_;
    std::wstring szSystemDir_;

    std::wstring getHostsPath();
    std::wstring getTempHostsPath();
	bool stringInVector(std::vector<std::wstring> &vec, std::wstring &str);
	std::vector<std::wstring> &split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems);
	std::vector<std::wstring> split(const std::wstring &s, wchar_t delim);
};

#endif // HOSTSEDIT_H
