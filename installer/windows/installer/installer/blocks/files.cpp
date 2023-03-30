#include "files.h"

#include <shlobj_core.h>

#include "../settings.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/directory.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../utils/utils.h"

using namespace std;

Files::Files(double weight) : IInstallBlock(weight, L"Files")
{
}

Files::~Files()
{
}

int Files::executeStep()
{
    if (state_ == 0)
    {
        // Since we're running as root, we need to ensure no malicious hackery can be done with symbolic links.
        // We'll install to the app's default, OS protected, 64-bit Program Files folder.  If the user specified
        // a custom install folder, we'll attempt to rename the install folder to the custom folder after the
        // file extraction is complete.
        installPath_ = Utils::defaultInstallPath();

        if (::SHCreateDirectoryEx(NULL, installPath_.c_str(), NULL) != ERROR_SUCCESS)
        {
            if (::GetLastError() != ERROR_ALREADY_EXISTS)
            {
                lastError_ = L"Can't create install directory";
                return -1;
            }
        }

        archive_.reset(new Archive(L"Windscribe"));

        SRes res = archive_->fileList(fileList_);

        if (res != SZ_OK)
        {
            lastError_ = L"Can't get files list from archive.";
            return -1;
        }

        fileList_.remove_if(FilterFiles(true, true));
        fillPathList();

        archive_->calcTotal(fileList_, pathList_);
        curFileInd_ = 0;
        state_++;
        return 0;
    }

    SRes res = archive_->extractionFile(curFileInd_);
    if (res != SZ_OK)
    {
        archive_->finish();
        lastError_ = L"Can't extract file.";
        return -1;
    }

    if (curFileInd_ >= (archive_->getNumFiles() - 1))
    {
        archive_->finish();
        return moveFiles();
    }

    int progress = ((double)curFileInd_ / (double)archive_->getNumFiles()) * 100.0;
    curFileInd_++;

    return progress;
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

wstring Files::getFileName(const wstring& s)
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

int Files::moveFiles()
{
    const wstring& settingsInstallPath = Settings::instance().getPath();

    if (Directory::caseInsensitiveStringCompare(installPath_, settingsInstallPath)) {
        return 100;
    }

    Log::instance().out(L"Moving installed files from [%s] to [%s]...", installPath_.c_str(), settingsInstallPath.c_str());

    try
    {
        // Delete the target folder just in case a symlink has been created on it, and because MoveFileEx() expects it to not exist.
        // The target folder, if it exists, is expected to be empty (i.e. we do not allow installing the app into a folder already
        // containing other files).  RemoveDirectory will fail if the target folder is not empty.
        if (Directory::DirExists(settingsInstallPath)) {
            if (::RemoveDirectory(settingsInstallPath.c_str()) == FALSE) {
                throw system_error(::GetLastError(), generic_category(), "could not remove target folder");
            }
        }
        else {
            // Ensure the target's parent folder exists or MoveFile will fail.
            wstring parentFolder = Path::PathExtractDir(settingsInstallPath);
            if (!parentFolder.empty() && !Path::isRoot(parentFolder) && !Directory::DirExists(parentFolder)) {
                Log::instance().out(L"Creating parent folder %s...", parentFolder.c_str());
                if (::SHCreateDirectoryEx(NULL, parentFolder.c_str(), NULL) != ERROR_SUCCESS) {
                    throw system_error(::GetLastError(), generic_category(), "could not create target folder's parent");
                }
            }
        }

        // MoveFileEx cannot be performed atomically if the source and target folders are on different volumes.
        // Thus, the installer only permits installation to a folder on the system drive.
        BOOL result = ::MoveFileEx(installPath_.c_str(), settingsInstallPath.c_str(), MOVEFILE_WRITE_THROUGH);
        if (result == FALSE) {
            throw system_error(::GetLastError(), generic_category(), "could not move the installed files");
        }
    }
    catch (system_error& ex) {
        // Update the install path that will be used by the subsequent blocks.
        Settings::instance().setPath(installPath_);
        Log::instance().out("Files::moveFiles() %s (%lu)", ex.what(), ex.code());
    }

    return 100;
}

FilterFiles::FilterFiles(bool isX64, bool win10_or_greater) :
    isX64_(isX64), win10_or_greater_(win10_or_greater)
{
}

bool FilterFiles::operator()(const std::wstring& value) const
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
