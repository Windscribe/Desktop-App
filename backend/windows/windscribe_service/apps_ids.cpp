#include "all_headers.h"
#include "apps_ids.h"
#include "logger.h"

AppsIds::AppsIds()
{
}

AppsIds::AppsIds(const AppsIds &a)
{
    addFromAppsImpl(a);
}

AppsIds::~AppsIds()
{
    clear();
}

void AppsIds::setFromList(const std::vector<std::wstring> &apps)
{
    clear();
    addFromListImpl(apps);
}

void AppsIds::addFrom(const AppsIds &a)
{
    addFromAppsImpl(a);
}

size_t AppsIds::count() const
{
    return appIds_.size();
}

FWP_BYTE_BLOB *AppsIds::getAppId(size_t ind)
{
    if (ind < appIds_.size())
    {
        return &appIds_[ind];
    }
    else
    {
        return NULL;
    }
}

bool AppsIds::operator==(const AppsIds& other) const
{
    if (appIds_.size() != other.appIds_.size())
    {
        return false;
    }

    for (size_t i = 0; i < appIds_.size(); ++i)
    {
        if (appIds_[i].size != other.appIds_[i].size)
        {
            return false;
        }

        if (appIds_[i].data != NULL && other.appIds_[i].data != NULL)
        {
            if (memcmp(appIds_[i].data, other.appIds_[i].data, appIds_[i].size) != 0)
            {
                return false;
            }
        }
    }

    return true;
}

AppsIds& AppsIds::operator = (const AppsIds &a)
{
    clear();
    addFromAppsImpl(a);
    return *this;
}

void AppsIds::clear()
{
    for (size_t i = 0; i < appIds_.size(); ++i)
    {
        delete[] appIds_[i].data;
    }
    appIds_.clear();
}

void AppsIds::addFromListImpl(const std::vector<std::wstring> &apps)
{
    for (auto it = apps.begin(); it != apps.end(); ++it)
    {
        FWP_BYTE_BLOB *fwpApplicationByteBlob = NULL;
        DWORD result = FwpmGetAppIdFromFileName(it->c_str(), &fwpApplicationByteBlob);
        if (result == ERROR_SUCCESS)
        {
            FWP_BYTE_BLOB blob;
            blob.size = fwpApplicationByteBlob->size;
            blob.data = new UINT8[blob.size];
            memcpy(blob.data, fwpApplicationByteBlob->data, blob.size);

            appIds_.push_back(blob);

            FwpmFreeMemory((void **)&fwpApplicationByteBlob);
        }
        else {
            Logger::instance().out(L"FwpmGetAppIdFromFileName(%s) failed: %d", it->c_str(), result);
        }
    }
}

void AppsIds::addFromAppsImpl(const AppsIds &a)
{
    for (size_t i = 0; i < a.appIds_.size(); ++i)
    {
        FWP_BYTE_BLOB blob;
        blob.size = a.appIds_[i].size;
        blob.data = new UINT8[blob.size];
        memcpy(blob.data, a.appIds_[i].data, blob.size);

        appIds_.push_back(blob);
    }
}
