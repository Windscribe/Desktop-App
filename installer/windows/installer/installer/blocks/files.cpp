#include "files.h"

#include <filesystem>
#include <shlobj_core.h>

#include "../installer_base.h"
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

        if (filesystem::exists(installPath_)) {
            if (!filesystem::is_empty(installPath_)) {
                Log::instance().out(L"Warning: the default install directory exists and is not empty.");
            }
        }
        else {
            auto result = ::SHCreateDirectoryEx(NULL, installPath_.c_str(), NULL);
            if (result != ERROR_SUCCESS) {
                Log::instance().out(L"Failed to create default install directory (%d)", result);
                return -ERROR_OTHER;
            }
        }

        archive_.reset(new Archive(L"Windscribe"));
        archive_->setLogFunction([](const char* str) {
            Log::instance().out((std::string("(archive) ") + str).c_str());
        });

        SRes res = archive_->fileList(fileList_);

        if (res != SZ_OK) {
            Log::instance().out(L"Failed to extract file list from archive.");
            return -ERROR_OTHER;
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
        return -ERROR_OTHER;
    }

    if (curFileInd_ >= (archive_->getNumFiles() - 1))
    {
        archive_->finish();
        if (!copyLibs()) {
            Log::instance().out(L"Failed to copy libs");
            return -ERROR_OTHER;
        }
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
        Log::instance().out(L"Could not move installed files: %hs", ex.what());

        // Delete "C:\Program Files\Windscribe" since we don't want to leave files behind.
        // SHFileOperation requires the path to be double-null terminated.
        std::wstring installPathDoubleNull = installPath_ + L"\0"s;
        SHFILEOPSTRUCT fileOp = {
            NULL,
            FO_DELETE,
            installPathDoubleNull.c_str(),
            NULL,
            FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
            FALSE,
            NULL,
            NULL
        };
        int ret = SHFileOperation(&fileOp);
        if (ret) {
            Log::instance().out(L"Could not delete partial install: %lu", ret);
        }
        return -ERROR_MOVE_CUSTOM_DIR;
    }

    return 100;
}

bool Files::copyLibs()
{
    std::error_code ec;
    std::filesystem::copy_options opts = std::filesystem::copy_options::overwrite_existing;

    const filesystem::path installPath = installPath_;
    const wstring exeStr = getExePath();
    if (exeStr.empty()) {
        Log::instance().out(L"Could not get exe path");
        return false;
    }
    const filesystem::path exePath = exeStr;

    // Copy DLLs
    for (const auto &entry : filesystem::directory_iterator(exePath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dll") {
            filesystem::copy_file(entry.path(), installPath / entry.path().filename(), opts, ec);
            if (ec) {
                Log::instance().out(L"Could not copy DLL %ls: %hs", entry.path().wstring().c_str(), ec.message().c_str());
                return false;
            }
        }
    }

    // Copy Qt plugins
    std::wstring paths[3] = { L"imageformats", L"platforms", L"styles" };
    for (auto p : paths) {
        filesystem::copy(exePath / p, installPath / p, opts, ec);
        if (ec) {
            Log::instance().out(L"Could not copy %ls: %hs", p.c_str(), ec.message().c_str());
            return false;
        }
    }
    return true;
}

wstring Files::getExePath()
{
    wchar_t path[MAX_PATH];
    int ret = GetModuleFileName(NULL, path, MAX_PATH);
    if (ret == 0) {
        return wstring();
    }
    return filesystem::path(path).parent_path().wstring();
}
