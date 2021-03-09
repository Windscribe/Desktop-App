#include "downloader.h"
#include <WinInet.h>
#include "../../utils/base64codec.h"
#include "blocks/ShellExecuteAsUser.h"

#pragma comment(lib, "Wininet.lib") 

namespace
{
// Base64 encoded legacy installer path:
// https://assets.totallyacdn.com/desktop/win/Windscribe_1.80.exe
const std::string kLegacyInstallerUrl =
    "aHR0cHM6Ly9hc3NldHMudG90YWxseWFjZG4uY29tL2Rlc2t0b3Avd2luL1dpbmRzY3JpYmVfMS44MC5leGU=";
const std::wstring kTempInstallerName = L"WindscribeLegacyInstaller.exe";
const DWORD kDefaultInstallerSize = 16518584u;
}

Downloader::Downloader(
    const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState)
    : InstallerBase(callbackState)
{
    const auto decodedUrl = Base64Codec::decode(kLegacyInstallerUrl);
    legacyInstallerUrl_ = std::wstring(decodedUrl.begin(), decodedUrl.end());
}

Downloader::~Downloader()
{
}

void Downloader::startImpl(HWND hwnd, const Settings &settings)
{
    TCHAR buffer[MAX_PATH + 1];
    GetTempPath(sizeof(buffer) / sizeof(buffer[0]), buffer);
    tempInstallerPath_ = std::wstring(buffer) + L"/" + kTempInstallerName;
}

void Downloader::executionImpl()
{
    // Request information about the legacy installer.
    auto hInternet = InternetOpen(L"WindscribeInstaller", INTERNET_OPEN_TYPE_PRECONFIG,
                                  nullptr, nullptr, 0);
    if (!hInternet) {
        strLastError_ = L"Can't open WinInet connection.";
        callbackState_(0, STATE_FATAL_ERROR);
        return;
    }

    // This is used to force reloading of the file, instead of using the cache.
    // If we are okay with caching, then it can be commented out.
#if defined(_DEBUG)
    DeleteUrlCacheEntry(legacyInstallerUrl_.c_str());
#endif

    const auto flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
        INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_SECURE | INTERNET_FLAG_DONT_CACHE |
        INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    auto hSession = InternetOpenUrl(hInternet, legacyInstallerUrl_.c_str(), nullptr, 0, 0, flags);
    if (!hSession) {
        InternetCloseHandle(hInternet);
        strLastError_ = L"Can't create WinInet session.";
        callbackState_(0, STATE_FATAL_ERROR);
        return;
    }

    char queryBuffer[32] = {};
    DWORD querySize = sizeof(queryBuffer);
    DWORD dataSize = kDefaultInstallerSize, index = 0;
    // This may fail, in this case use a hardcoded installer size. It is only needed to estimate the
    // download progress. Use A-version, because we expect it to be an ASCII string, not wide chars.
    if (HttpQueryInfoA(hSession, HTTP_QUERY_CONTENT_LENGTH, queryBuffer, &querySize, &index))
        dataSize = static_cast<DWORD>(atol(queryBuffer));

    // Create a temporary installer file.
    auto hFile = CreateFile(tempInstallerPath_.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hSession);
        InternetCloseHandle(hInternet);
        strLastError_ = L"Can't create temporary file: " + tempInstallerPath_;
        callbackState_(0, STATE_ERROR);
        return;
    }

    callbackState_(0, STATE_EXTRACTING);

    // Read the file.
    char dataBuffer[8192] = {};
    DWORD dataTotal = 0, dataRead = 0, dataWrite = 0;
    DWORD lastProgress = 0;
    bool writeError = false;
    while (InternetReadFile(hSession, dataBuffer, sizeof(dataBuffer), &dataRead) && dataRead) {
        dataTotal += dataRead;
        if (!WriteFile(hFile, dataBuffer, dataRead, &dataWrite, nullptr) || !dataWrite) {
            writeError = true;
            break;
        }
        const DWORD progress = min(100, static_cast<DWORD>(dataTotal * 100 / dataSize));
        if (progress > lastProgress) {
            lastProgress = progress;
            callbackState_(progress, STATE_EXTRACTING);
        }
    }
    CloseHandle(hFile);

    if (writeError) {
        DeleteFile(tempInstallerPath_.c_str());
        strLastError_ = L"Can't write temporary file: " + tempInstallerPath_;
        callbackState_(0, STATE_ERROR);
        return;
    } else {
        callbackState_(100u, STATE_FINISHED);
    }

    InternetCloseHandle(hSession);
    InternetCloseHandle(hInternet);
}

void Downloader::runLauncherImpl()
{
    ShellExecuteAsUser::shellExecuteFromExplorer(tempInstallerPath_.c_str(), nullptr, nullptr,
                                                 nullptr, SW_RESTORE);
    // Make sure the legacy installer will be deleted after reboot.
    MoveFileEx(tempInstallerPath_.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
}
