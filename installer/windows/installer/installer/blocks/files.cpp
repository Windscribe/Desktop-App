#include "files.h"

#include <filesystem>
#include <shlobj_core.h>
#include <spdlog/spdlog.h>

#include "../installer_base.h"
#include "../settings.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/archive.h"
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
    // Since we're running as root, we need to ensure no malicious hackery can be done with symbolic links.
    // We'll install to the app's default, OS protected, 64-bit Program Files folder.  If the user specified
    // a custom install folder, we'll attempt to rename the install folder to the custom folder after the
    // file extraction is complete.
    installPath_ = ApplicationInfo::defaultInstallPath();

    error_code ec;
    if (filesystem::exists(installPath_, ec)) {
        if (!filesystem::is_empty(installPath_, ec)) {
            if (ec) {
                spdlog::error("Files::executeStep: filesystem::is_empty failed ({}).", ec.message().c_str());
            }
            spdlog::warn(L"Warning: the default install directory exists and is not empty.");
        }
    }
    else {
        if (ec) {
            spdlog::error("Files::executeStep: filesystem::exists failed ({}).", ec.message().c_str());
        }

        auto result = ::SHCreateDirectoryEx(NULL, installPath_.c_str(), NULL);
        if (result != ERROR_SUCCESS) {
            spdlog::error(L"Failed to create default install directory ({})", result);
            return -ERROR_OTHER;
        }
    }

    const wstring exePath = Utils::getExePath();
    if (exePath.empty()) {
        spdlog::error(L"Could not get exe path");
        return -ERROR_OTHER;
    }

    wsl::Archive archive;
    archive.setLogFunction([](const wstring &str) {
        spdlog::info(L"{}", str);
    });

    if (!archive.extract(L"Windscribe", L"windscribe.7z", exePath, installPath_)) {
        return -ERROR_OTHER;
    }

    if (!copyLibs()) {
        spdlog::error(L"Failed to copy libs");
        return -ERROR_OTHER;
    }

    return moveFiles();
}

int Files::moveFiles()
{
    const wstring& settingsInstallPath = Settings::instance().getPath();

    try {
        if (Path::equivalent(installPath_, settingsInstallPath)) {
            return 100;
        }

        spdlog::info(L"Moving installed files to user custom path...");

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
                spdlog::info(L"Creating parent folder...");
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
        spdlog::error("Could not move installed files: {}", ex.what());

        // Delete "C:\Program Files\Windscribe" since we don't want to leave files behind.
        // SHFileOperation requires the path to be double-null terminated.
        wstring installPathDoubleNull = installPath_ + L"\0"s;
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
            spdlog::error(L"Could not delete partial install: {}", ret);
        }
        return -ERROR_MOVE_CUSTOM_DIR;
    }

    return 100;
}

bool Files::copyLibs()
{
    error_code ec;
    filesystem::copy_options opts = filesystem::copy_options::overwrite_existing;

    const filesystem::path installPath = installPath_;
    const wstring exeStr = Utils::getExePath();
    if (exeStr.empty()) {
        spdlog::error(L"Could not get exe path");
        return false;
    }
    const filesystem::path exePath = exeStr;

    // Copy DLLs
    for (const auto &entry : filesystem::directory_iterator(exePath, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dll") {
            filesystem::copy_file(entry.path(), installPath / entry.path().filename(), opts, ec);
            if (ec) {
                spdlog::error(L"Could not copy DLL {}, error {}", entry.path().c_str(), ec.value());
                return false;
            }
        }
    }

    if (ec) {
        spdlog::error("Files::copyLibs: filesystem::directory_iterator failed ({})", ec.message().c_str());
        return false;
    }

    // Copy Qt plugins
    wstring paths[3] = { L"imageformats", L"platforms", L"styles" };
    for (auto p : paths) {
        filesystem::copy(exePath / p, installPath / p, opts, ec);
        if (ec) {
            spdlog::error(L"Could not copy {}, error {}", p.c_str(), ec.value());
            return false;
        }
    }

    return true;
}
