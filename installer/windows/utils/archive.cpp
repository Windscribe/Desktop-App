#include "archive.h"

#include <codecvt>
#include <filesystem>
#include <sstream>
#include <system_error>

#include <windows.h>

#include "win32handle.h"
#include "wsscopeguard.h"

namespace wsl {

static std::wstring to_wstring(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

Archive::Archive()
{
}

void Archive::setLogFunction(const std::function<void (const std::wstring &)> &func)
{
    logFunction_ = func;
}

void Archive::log(const std::wstring& msg)
{
    if (logFunction_) {
        logFunction_(msg);
    }
}

std::wstring Archive::extractor(const std::wstring &folder) const
{
    std::filesystem::path extractorUtility(folder);
    extractorUtility.append(L"7zr.exe");
    return extractorUtility;
}

bool Archive::extract(const std::wstring &resourceName, const std::wstring &archiveName,
                      const std::wstring &extractFolder, const std::wstring &targetFolder)
{
    bool isExtracted = false;

    try {
        if (!std::filesystem::exists(extractor(extractFolder))) {
            extractArchiveFromResource(L"7zExtractor", L"7zr.exe", extractFolder);
        }

        extractArchiveFromResource(resourceName, archiveName, extractFolder);
        extractFiles(archiveName, extractFolder, targetFolder);
        isExtracted = true;
    }
    catch (std::system_error &ex) {
        log(to_wstring(ex.what()));
    }

    return isExtracted;
}

bool Archive::extract(const std::wstring &resourceName, const std::wstring &archiveName, const std::wstring &extractFolder,
                      const std::wstring &filename, const std::wstring &targetFolder)
{
    bool isExtracted = false;

    try {
        // The extractor utility is expected to exist in the extractFolder, placed there by the bootstrapper.
        if (!std::filesystem::exists(extractor(extractFolder))) {
            log(L"The extraction utility was not found in " + extractFolder);
            return false;
        }

        extractArchiveFromResource(resourceName, archiveName, extractFolder);
        extractFile(archiveName, extractFolder, filename, targetFolder);
        isExtracted = true;
    }
    catch (std::system_error &ex) {
        log(to_wstring(ex.what()));
    }

    return isExtracted;
}

void Archive::extractArchiveFromResource(const std::wstring &resourceName, const std::wstring &archiveName, const std::wstring &extractFolder)
{
    log(L"Extracting resource: " + archiveName);

    HRSRC resource = ::FindResource(nullptr, resourceName.c_str(), L"BINARY");
    if (resource == NULL) {
        throw std::system_error(::GetLastError(), std::system_category(), "extractArchiveFromResource FindResource failed");
    }

    HGLOBAL dataHandle = ::LoadResource(NULL, resource);
    if (dataHandle == NULL) {
        throw std::system_error(::GetLastError(), std::system_category(), "extractArchiveFromResource LoadResource failed");
    }

    size_t dataSize = ::SizeofResource(NULL, resource);
    if (dataSize == 0) {
        throw std::system_error(::GetLastError(), std::system_category(), "extractArchiveFromResource SizeofResource failed");
    }

    LPVOID data = ::LockResource(dataHandle);
    if (data == NULL) {
        throw std::system_error(::GetLastError(), std::system_category(), "extractArchiveFromResource LockResource failed");
    }

    std::filesystem::path target(extractFolder);
    std::filesystem::create_directories(target);
    target.append(archiveName);

    FILE* fileHandle = nullptr;
    auto closeFile = wsl::wsScopeGuard([&] {
        if (fileHandle != nullptr) {
            fclose(fileHandle);
        }
    });

    errno_t result = _wfopen_s(&fileHandle, target.c_str(), L"wb");
    if ((result != 0) || (fileHandle == nullptr)) {
        throw std::system_error(result, std::system_category(), "extractArchiveFromResource _wfopen_s failed");
    }

    size_t bytesWritten = fwrite(data, 1, dataSize, fileHandle);
    if (bytesWritten != dataSize) {
        throw std::system_error(errno, std::system_category(), "extractArchiveFromResource fwrite failed to write entire resource");
    }

    fflush(fileHandle);
}

void Archive::extractFile(const std::wstring &archiveName, const std::wstring &extractFolder,
                          const std::wstring &filename, const std::wstring &targetFolder)
{
    log(L"Extracting " + filename + L" from " + archiveName);

    std::filesystem::path archive(extractFolder);
    archive.append(archiveName);

    std::wostringstream cmd;
    cmd << L"\"" << extractor(extractFolder) << L"\" e -y -bb3 -bd -o\"" << targetFolder << "\" \"" << archive.c_str() << L"\" " << filename;

    executeCmd(cmd.str());
}

void Archive::extractFiles(const std::wstring &archiveName, const std::wstring &extractFolder, const std::wstring &targetFolder)
{
    log(L"Extracting files from " + archiveName);

    std::filesystem::path archive(extractFolder);
    archive.append(archiveName);

    std::wostringstream cmd;
    cmd << L"\"" << extractor(extractFolder) << L"\" x -y -bb3 -bd -o\"" << targetFolder << "\" \"" << archive.c_str() << L"\"";

    executeCmd(cmd.str());
}

void Archive::executeCmd(const std::wstring &cmd)
{
    SECURITY_ATTRIBUTES sa;
    ::ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    Win32Handle pipeRead;
    Win32Handle pipeWrite;
    if (!::CreatePipe(pipeRead.data(), pipeWrite.data(), &sa, 0)) {
        throw std::system_error(::GetLastError(), std::system_category(), "executeCmd CreatePipe failed");
    }

    STARTUPINFO si;
    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = pipeWrite.getHandle();
    si.hStdOutput = pipeWrite.getHandle();

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    std::unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, cmd.c_str(), _TRUNCATE);

    log(L"executeCmd: " + cmd);

    PROCESS_INFORMATION pi;
    ::ZeroMemory(&pi, sizeof(pi));
    auto result = ::CreateProcess(NULL, exec.get(), NULL, NULL, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);

    if (result == FALSE) {
        throw std::system_error(::GetLastError(), std::system_category(), "executeCmd CreateProcess failed");
    }

    ::CloseHandle(pi.hThread);
    Win32Handle process(pi.hProcess);

    result = process.wait(INFINITE);
    if (result != WAIT_OBJECT_0) {
        throw std::system_error(::GetLastError(), std::system_category(), "executeCmd WaitForSingleObject failed");
    }

    DWORD exitCode = 1;
    result = ::GetExitCodeProcess(process.getHandle(), &exitCode);
    if (result == FALSE) {
        log(L"executeCmd GetExitCodeProcess failed: " + std::to_wstring(::GetLastError()));
    }

    pipeWrite.closeHandle();

    char buf[1024];
    std::string programOutput;
    while (true) {
        DWORD bytesRead;
        result = ::ReadFile(pipeRead.getHandle(), buf, 1024, &bytesRead, NULL);
        if (result == FALSE || bytesRead == 0) {
            break;
        }
        programOutput += std::string(buf, bytesRead);
    }

    if (!programOutput.empty()) {
        log(L"executeCmd output: " + to_wstring(programOutput));
    }

    if (exitCode != 0) {
        throw std::system_error(ERROR_INVALID_DATA, std::system_category(), "executeCmd process returned failure code " + std::to_string(exitCode));
    }
}

}
