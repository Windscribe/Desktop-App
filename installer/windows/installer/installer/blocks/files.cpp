#include "files.h"

#include <filesystem>
#include <shlobj_core.h>

#include "../settings.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"

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
        installPath_ = ApplicationInfo::defaultInstallPath();

        if (::SHCreateDirectoryEx(NULL, installPath_.c_str(), NULL) != ERROR_SUCCESS)
        {
            if (::GetLastError() != ERROR_ALREADY_EXISTS)
            {
                Log::instance().out(L"Failed to create install directory");
                return -1;
            }
        }

        archive_.reset(new Archive(L"Windscribe"));

        SRes res = archive_->fileList(fileList_);

        if (res != SZ_OK) {
            Log::instance().out(L"Failed to extract file list from archive.");
            return -1;
        }

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
        Log::instance().out(L"Failed to extract file at index %u.", curFileInd_);
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
    for (auto it = fileList_.cbegin(); it != fileList_.cend(); it++) {
        wstring file = Path::append(installPath_, *it);
        pathList_.push_back(Path::extractDir(file));
    }
}

int Files::moveFiles()
{
    const wstring& settingsInstallPath = Settings::instance().getPath();

    try {
        if (Path::equivalent(installPath_, settingsInstallPath)) {
            return 100;
        }

        Log::instance().out(L"Moving installed files to user custom path...");

        // Delete the target folder just in case a symlink has been created on it, and because MoveFileEx() expects it to not exist.
        // The target folder, if it exists, is expected to be empty (i.e. we do not allow installing the app into a folder already
        // containing other files).  RemoveDirectory will fail if the target folder is not empty.
        if (filesystem::exists(settingsInstallPath)) {
            if (::RemoveDirectory(settingsInstallPath.c_str()) == FALSE) {
                throw system_error(::GetLastError(), generic_category(), "could not remove target folder");
            }
        }
        else {
            // Ensure the target's parent folder exists or MoveFile will fail.
            wstring parentFolder = Path::extractDir(settingsInstallPath);
            if (!parentFolder.empty() && !Path::isRoot(parentFolder) && !filesystem::exists(parentFolder)) {
                Log::instance().out(L"Creating parent folder...");
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
        Log::instance().out("Files::moveFiles() %s (%lu)", ex.what(), ex.code().value());
    }

    return 100;
}
