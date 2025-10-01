#pragma once

#include <fwptypes.h>

class AppsIds
{
public:
    AppsIds();
    AppsIds(const AppsIds &a);
    ~AppsIds();

    void setFromList(const std::vector<std::wstring> &apps);
    void addFrom(const AppsIds &a);

    size_t count() const;
    const FWP_BYTE_BLOB *getAppId(size_t ind) const;

    bool operator==(const AppsIds& other) const;

    AppsIds& operator = (const AppsIds &a);

private:
    std::vector<FWP_BYTE_BLOB> appIds_;

    void clear();
    void addFromListImpl(const std::vector<std::wstring> &apps);
    void addFromAppsImpl(const AppsIds &a);
};

