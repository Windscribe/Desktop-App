#include "archive.h"

#include <filesystem>
#include <sstream>
#include <system_error>
#include <thread>

#include <windows.h>

#include "wide_string.h"
#include "win32handle.h"
#include "wsscopeguard.h"

namespace wsl {

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

bool Archive::extractDecompressor(const std::wstring &extractFolder)
{
    try {
        extractArchiveFromResource(L"7zExtractor", L"7zr.exe", extractFolder);
        return true;
    }
    catch (std::system_error &ex) {
        log(toWideString(ex.what()));
        return false;
    }
}

ArchiveResult Archive::extract(const std::wstring &resourceName, const std::wstring &archiveName,
                               const std::wstring &extractFolder, const std::wstring &targetFolder)
{
    ArchiveResult result;

    try {
        if (!std::filesystem::exists(extractor(extractFolder))) {
            result.status = ArchiveResult::Status::LaunchFailed;
            result.errorText = L"The extraction utility was not found in " + extractFolder;
            log(result.errorText);
            return result;
        }

        extractArchiveFromResource(resourceName, archiveName, extractFolder);
        return extractFiles(archiveName, extractFolder, targetFolder);
    }
    catch (std::system_error &ex) {
        result.status = ArchiveResult::Status::ExtractFailed;
        result.errorText = toWideString(ex.what());
        log(result.errorText);
        return result;
    }
}

ArchiveResult Archive::extract(const std::wstring &resourceName, const std::wstring &archiveName, const std::wstring &extractFolder,
                               const std::wstring &filename, const std::wstring &targetFolder)
{
    ArchiveResult result;

    try {
        // The extractor utility is expected to exist in the extractFolder, placed there by the bootstrapper.
        if (!std::filesystem::exists(extractor(extractFolder))) {
            result.status = ArchiveResult::Status::LaunchFailed;
            result.errorText = L"The extraction utility was not found in " + extractFolder;
            log(result.errorText);
            return result;
        }

        extractArchiveFromResource(resourceName, archiveName, extractFolder);
        return extractFile(archiveName, extractFolder, filename, targetFolder);
    }
    catch (std::system_error &ex) {
        result.status = ArchiveResult::Status::ExtractFailed;
        result.errorText = toWideString(ex.what());
        log(result.errorText);
        return result;
    }
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

ArchiveResult Archive::extractFile(const std::wstring &archiveName, const std::wstring &extractFolder,
                                   const std::wstring &filename, const std::wstring &targetFolder)
{
    log(L"Extracting " + filename + L" from " + archiveName);

    std::filesystem::path archive(extractFolder);
    archive.append(archiveName);

    std::wostringstream cmd;
    cmd << L"\"" << extractor(extractFolder) << L"\" e -y -bb3 -bd -o\"" << targetFolder << "\" \"" << archive.c_str() << L"\" " << filename;

    return executeCmd(cmd.str(), extractFolder);
}

ArchiveResult Archive::extractFiles(const std::wstring &archiveName, const std::wstring &extractFolder, const std::wstring &targetFolder)
{
    log(L"Extracting files from " + archiveName);

    std::filesystem::path archive(extractFolder);
    archive.append(archiveName);

    std::wostringstream cmd;
    cmd << L"\"" << extractor(extractFolder) << L"\" x -y -bb3 -bd -o\"" << targetFolder << "\" \"" << archive.c_str() << L"\"";

    return executeCmd(cmd.str(), extractFolder);
}

ArchiveResult Archive::executeCmd(const std::wstring &cmd, const std::wstring &workingDir)
{
    ArchiveResult opResult;

    SECURITY_ATTRIBUTES sa;
    ::ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    // 7z writes its progress/file listing to stdout and error messages to stderr.
    // Keep them in separate pipes so the error text can be reported as-is, without
    // having to parse it out of the (large) stdout stream.
    Win32Handle stdoutRead;
    Win32Handle stdoutWrite;
    if (!::CreatePipe(stdoutRead.data(), stdoutWrite.data(), &sa, 0)) {
        opResult.status = ArchiveResult::Status::ExtractFailed;
        opResult.errorText = L"executeCmd CreatePipe (stdout) failed: " + std::to_wstring(::GetLastError());
        log(opResult.errorText);
        return opResult;
    }

    Win32Handle stderrRead;
    Win32Handle stderrWrite;
    if (!::CreatePipe(stderrRead.data(), stderrWrite.data(), &sa, 0)) {
        opResult.status = ArchiveResult::Status::ExtractFailed;
        opResult.errorText = L"executeCmd CreatePipe (stderr) failed: " + std::to_wstring(::GetLastError());
        log(opResult.errorText);
        return opResult;
    }

    STARTUPINFO si;
    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = stderrWrite.getHandle();
    si.hStdOutput = stdoutWrite.getHandle();

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    std::unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, cmd.c_str(), _TRUNCATE);

    log(L"executeCmd: " + cmd);

    PROCESS_INFORMATION pi;
    ::ZeroMemory(&pi, sizeof(pi));
    // Explicitly pass the trusted (admin-only) workingDir as the child's CWD; otherwise 7zr.exe would
    // inherit the bootstrap's CWD (e.g. the user's Downloads folder), which is a DLL-planting vector.
    LPCWSTR cwd = workingDir.empty() ? NULL : workingDir.c_str();
    auto result = ::CreateProcess(NULL, exec.get(), NULL, NULL, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, cwd, &si, &pi);

    if (result == FALSE) {
        opResult.status = ArchiveResult::Status::LaunchFailed;
        opResult.errorText = L"executeCmd CreateProcess failed: " + std::to_wstring(::GetLastError());
        log(opResult.errorText);
        return opResult;
    }

    ::CloseHandle(pi.hThread);
    Win32Handle process(pi.hProcess);

    // Close our copies of the write ends now that the child holds its own inherited copies.
    // ReadFile on the read ends then returns EOF once the child exits (or closes its ends),
    // instead of blocking forever on our still-open write handles.
    stdoutWrite.closeHandle();
    stderrWrite.closeHandle();

    auto readPipe = [](HANDLE pipe) {
        char buf[1024];
        std::string output;
        while (true) {
            DWORD bytesRead;
            if (::ReadFile(pipe, buf, sizeof(buf), &bytesRead, NULL) == FALSE || bytesRead == 0) {
                break;
            }
            output.append(buf, bytesRead);
        }
        return output;
    };

    // Drain both pipes to EOF *while the child runs*, and only wait for it to exit
    // afterwards.  Waiting first would deadlock: a pipe's buffer is only a few KB, and
    // with -bb3 7z logs every extracted file, so once the buffer fills the child blocks
    // in write and never exits.  stderr is drained on its own thread for the same
    // reason: the child stalls if either pipe fills while we are reading the other one.
    std::string errorOutput;
    std::thread stderrReader([&errorOutput, &readPipe, &stderrRead] {
        errorOutput = readPipe(stderrRead.getHandle());
    });
    const std::string programOutput = readPipe(stdoutRead.getHandle());
    stderrReader.join();

    result = process.wait(INFINITE);
    if (result != WAIT_OBJECT_0) {
        opResult.status = ArchiveResult::Status::ExtractFailed;
        opResult.errorText = L"executeCmd WaitForSingleObject failed: " + std::to_wstring(::GetLastError());
        log(opResult.errorText);
        return opResult;
    }

    DWORD exitCode = 1;
    result = ::GetExitCodeProcess(process.getHandle(), &exitCode);
    if (result == FALSE) {
        log(L"executeCmd GetExitCodeProcess failed: " + std::to_wstring(::GetLastError()));
    }

    if (!programOutput.empty()) {
        log(L"executeCmd output: " + toWideString(programOutput));
    }

    if (!errorOutput.empty()) {
        log(L"executeCmd error output: " + toWideString(errorOutput));
    }

    if (exitCode != 0) {
        opResult.status = ArchiveResult::Status::ExtractFailed;
        opResult.exitCode = exitCode;
        opResult.errorText = toWideString(errorOutput);
        log(L"executeCmd process returned failure code " + std::to_wstring(exitCode));
    }

    return opResult;
}

}
