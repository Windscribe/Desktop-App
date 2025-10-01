#include "files.h"

#include <QStringList>

#include <filesystem>
#include <shlobj_core.h>
#include <spdlog/spdlog.h>

#include "installerenums.h"
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
                spdlog::error("Files::executeStep: filesystem::is_empty failed ({}).", ec.message());
            }
            spdlog::warn(L"Warning: the default install directory exists and is not empty.");
        }
    }
    else {
        if (ec) {
            spdlog::error("Files::executeStep: filesystem::exists failed ({}).", ec.message());
        }

        auto result = ::SHCreateDirectoryEx(NULL, installPath_.c_str(), NULL);
        if (result != ERROR_SUCCESS) {
            spdlog::error(L"Failed to create default install directory ({})", result);
            return -wsl::ERROR_OTHER;
        }
    }

    const wstring exePath = Utils::getExePath();
    if (exePath.empty()) {
        spdlog::error(L"Could not get exe path");
        return -wsl::ERROR_OTHER;
    }

    wsl::Archive archive;
    archive.setLogFunction([](const wstring &str) {
        spdlog::info(L"{}", str);
    });

    if (!archive.extract(L"Windscribe", L"windscribe.7z", exePath, installPath_)) {
        return -wsl::ERROR_OTHER;
    }

    return moveFiles();
}

int Files::moveFiles()
{
    int result = -wsl::ERROR_MOVE_CUSTOM_DIR;
    const wstring settingsInstallPath = Settings::instance().getPath();

    try {
        if (Path::equivalent(installPath_, settingsInstallPath)) {
            return 100;
        }

        spdlog::info(L"Moving installed files to user custom path...");

        // Delete the target folder just in case a symlink has been created on it, and because MoveFileEx() expects it to not exist.
        // The target folder, if it exists, is expected to be empty (i.e. we do not allow installing the app into a folder already
        // containing other files).  RemoveDirectory will fail if the target folder is not empty.
        if (filesystem::exists(settingsInstallPath)) {
            if (!filesystem::is_empty(settingsInstallPath)) {
                logCustomFolderContents(settingsInstallPath);
                result = -wsl::ERROR_CUSTOM_DIR_NOT_EMPTY;
                throw system_error(ERROR_DIR_NOT_EMPTY, generic_category(), "target folder is not empty");
            }
            if (::RemoveDirectory(settingsInstallPath.c_str()) == FALSE) {
                result = -wsl::ERROR_DELETE_CUSTOM_DIR;
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
        BOOL filesMoved = ::MoveFileEx(installPath_.c_str(), settingsInstallPath.c_str(), MOVEFILE_WRITE_THROUGH);
        if (filesMoved == FALSE) {
            throw system_error(::GetLastError(), generic_category(), "could not move the installed files");
        }

        result = 100;
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
            spdlog::error(L"Could not delete partial install from default folder: {}", ret);
        }
    }

    return result;
}

void Files::logCustomFolderContents(const wstring &folder)
{
    QStringList sl;
    error_code ec;
    for (const auto &entry : filesystem::recursive_directory_iterator(folder, filesystem::directory_options::skip_permission_denied, ec)) {
        sl.append(QString::fromStdWString(std::filesystem::relative(entry.path(), folder)));
    }

    if (ec) {
        spdlog::error("Files::logCustomFolderContents: filesystem::recursive_directory_iterator failed ({})", ec.message());
        return;
    }

    QString fileList = (sl.empty() ? "** no files found **" : sl.join(", "));
    spdlog::error(L"The custom install folder is not empty.  It contains the following files: {}", fileList.toStdWString());
}
