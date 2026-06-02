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

    bool addHosts(const std::wstring &ip, const std::wstring &hostname);
    bool makeHostsFileWritable() const;
    bool removeHosts();

private:
    std::wstring addedByMarker_;
    std::wstring systemDir_;

    HostsEdit();
    virtual ~HostsEdit();
    std::wstring getHostsPath() const;
    std::wstring getTempHostsPath() const;
    bool stringInVector(const std::vector<std::wstring> &vec, const std::wstring &str);
};
