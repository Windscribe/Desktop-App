#include "payloadextractor.h"

#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <windows.h>

#include "wide_string.h"
#include "win32handle.h"
#include "wsscopeguard.h"

namespace wsl {

namespace {

// Large enough to keep the WriteFile count on the largest app binary in the hundreds, and immaterial
// next to the LZMA dictionary the decoder itself allocates.
constexpr size_t kCopyBufferSize = 256 * 1024;

[[noreturn]] void throwArchiveError(archive *reader, const char *operation)
{
    const char *detail = archive_error_string(reader);
    throw std::runtime_error(std::string(operation) + " failed: " + ((detail != nullptr) ? detail : "no further detail"));
}

}

void PayloadExtractor::setLogFunction(const std::function<void (const std::wstring &)> &func)
{
    logFunction_ = func;
}

void PayloadExtractor::log(const std::wstring& msg) const
{
    if (logFunction_) {
        logFunction_(msg);
    }
}

ArchiveResult PayloadExtractor::extract(const std::wstring &resourceName, const std::filesystem::path &targetFolder)
{
    return extractResource(resourceName, nullptr, targetFolder);
}

ArchiveResult PayloadExtractor::extract(const std::wstring &resourceName, const std::wstring &filename,
                                        const std::filesystem::path &targetFolder)
{
    return extractResource(resourceName, &filename, targetFolder);
}

ArchiveResult PayloadExtractor::extractResource(const std::wstring &resourceName, const std::wstring *filename,
                                       const std::filesystem::path &targetFolder)
{
    ArchiveResult result;

    try {
        HRSRC resource = ::FindResource(nullptr, resourceName.c_str(), L"BINARY");
        if (resource == NULL) {
            throw std::system_error(::GetLastError(), std::system_category(), "FindResource failed");
        }

        HGLOBAL dataHandle = ::LoadResource(NULL, resource);
        if (dataHandle == NULL) {
            throw std::system_error(::GetLastError(), std::system_category(), "LoadResource failed");
        }

        const DWORD dataSize = ::SizeofResource(NULL, resource);
        if (dataSize == 0) {
            throw std::system_error(::GetLastError(), std::system_category(), "SizeofResource failed");
        }

        // Resource data stays mapped for the life of the process, so there is nothing to unlock or free afterwards.
        const LPVOID data = ::LockResource(dataHandle);
        if (data == NULL) {
            throw std::system_error(::GetLastError(), std::system_category(), "LockResource failed");
        }

        log(L"Extracting " + ((filename != nullptr) ? *filename : std::wstring(L"all files")) + L" from resource " + resourceName +
            L" (" + std::to_wstring(dataSize) + L" bytes) to " + targetFolder.native());

        extractFromMemory(data, dataSize, filename, targetFolder);
    }
    catch (const std::exception &ex) {
        result.succeeded = false;
        result.errorText = toWideString(ex.what());
        log(result.errorText);
    }

    return result;
}

void PayloadExtractor::extractFromMemory(const void *data, size_t size, const std::wstring *filename,
                                         const std::filesystem::path &targetFolder)
{
    std::filesystem::create_directories(targetFolder);

    archive *reader = archive_read_new();
    if (reader == nullptr) {
        throw std::runtime_error("archive_read_new failed");
    }

    auto freeReader = wsl::wsScopeGuard([&reader] {
        archive_read_free(reader);
    });

    // The 7z format reader drives its own decoders (LZMA/LZMA2 and the executable branch filters), so no
    // read filters are needed.  Registering only this format keeps every other parser out of an elevated process.
    if (archive_read_support_format_7zip(reader) != ARCHIVE_OK) {
        throwArchiveError(reader, "archive_read_support_format_7zip");
    }

    if (archive_read_open_memory(reader, data, size) != ARCHIVE_OK) {
        throwArchiveError(reader, "archive_read_open_memory");
    }

    // Anything but ARCHIVE_OK, including the warnings libarchive raises for a damaged archive, fails:
    // a payload that is only partly readable is not one to install part of.
    bool fileFound = false;
    int result = ARCHIVE_OK;
    archive_entry *entry = nullptr;
    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK) {
        const wchar_t *wideName = archive_entry_pathname_w(entry);
        const char *narrowName = archive_entry_pathname(entry);
        const std::wstring name = (wideName != nullptr) ? wideName : toWideString((narrowName != nullptr) ? narrowName : "");

        if ((filename != nullptr) && (name != *filename)) {
            if (archive_read_data_skip(reader) != ARCHIVE_OK) {
                throwArchiveError(reader, "archive_read_data_skip");
            }
            continue;
        }

        extractEntry(reader, entry, name, targetFolder);
        fileFound = true;
    }

    if (result != ARCHIVE_EOF) {
        throwArchiveError(reader, "archive_read_next_header");
    }

    if (archive_read_close(reader) != ARCHIVE_OK) {
        throwArchiveError(reader, "archive_read_close");
    }

    if ((filename != nullptr) && !fileFound) {
        throw std::runtime_error("the requested file was not found in the archive");
    }
}

void PayloadExtractor::extractEntry(archive *reader, archive_entry *entry, const std::wstring &name,
                                    const std::filesystem::path &targetFolder) const
{
    const std::filesystem::path entryPath(name);

    // This runs elevated and writes into Program Files, so a name that could resolve outside the target folder is
    // refused rather than sanitised: our own build produces the archive, so such a name means it has been tampered with.
    const bool escapesTargetFolder = name.empty() || entryPath.has_root_name() || entryPath.has_root_directory() ||
        std::any_of(entryPath.begin(), entryPath.end(), [](const std::filesystem::path &part) { return part == L".."; });

    if (escapesTargetFolder) {
        log(L"Refusing archive entry '" + name + L"': not a relative path within the target folder");
        throw std::runtime_error("archive entry is not a relative path within the target folder");
    }

    const std::filesystem::path target = targetFolder / entryPath;

    switch (archive_entry_filetype(entry)) {
    case AE_IFDIR:
        log(L"Creating " + target.native());
        std::filesystem::create_directories(target);
        break;

    case AE_IFREG:
        log(L"Extracting " + target.native());
        std::filesystem::create_directories(target.parent_path());
        writeFile(reader, target, archive_entry_size_is_set(entry) ? archive_entry_size(entry) : -1);
        break;

    default:
        // The archive is built from plain files and folders, so symlinks, reparse points or devices are as unexpected
        // as a name that escapes the folder.
        log(L"Refusing archive entry '" + name + L"': not a file or a directory");
        throw std::runtime_error("archive entry is not a file or a directory");
    }
}

void PayloadExtractor::writeFile(archive *reader, const std::filesystem::path &target, int64_t expectedSize) const
{
    Win32Handle file(::CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file.isValid()) {
        throw std::system_error(::GetLastError(), std::system_category(), "CreateFileW failed");
    }

    std::vector<char> buffer(kCopyBufferSize);
    int64_t bytesExtracted = 0;

    while (true) {
        // Verifies the entry's stored CRC as it goes, and fails at the end of an entry that does not match.
        const la_ssize_t bytesRead = archive_read_data(reader, buffer.data(), buffer.size());
        if (bytesRead == 0) {
            break;
        }

        if (bytesRead < 0) {
            throwArchiveError(reader, "archive_read_data");
        }

        DWORD bytesWritten = 0;
        if (!::WriteFile(file.getHandle(), buffer.data(), static_cast<DWORD>(bytesRead), &bytesWritten, nullptr)) {
            throw std::system_error(::GetLastError(), std::system_category(), "WriteFile failed");
        }

        if (bytesWritten != static_cast<DWORD>(bytesRead)) {
            throw std::runtime_error("WriteFile did not write the whole buffer");
        }

        bytesExtracted += bytesRead;
    }

    // A short file means something went wrong that neither libarchive nor the writes above reported.
    if ((expectedSize >= 0) && (bytesExtracted != expectedSize)) {
        throw std::runtime_error("extracted file is not the size the archive declared for it");
    }
}

}
