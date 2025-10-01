#include "crashdump.h"
#include <Windows.h>
#include <dbghelp.h>

#if defined(WINDSCRIBE_SERVICE)
#include <spdlog/spdlog.h>
#endif

#if defined(ENABLE_CRASH_REPORTS)

namespace Debug {

class CrashDumpInternal
{
public:
    CrashDumpInternal();
    ~CrashDumpInternal();

    bool isAvailable() const;
    void setApiVersion() const;
    void setPrivileges() const;
    bool writeMinidump(const std::wstring &filename, DWORD thread_id,
                       PEXCEPTION_POINTERS exception_pointers);

private:
    using PFNIMAGEHLPAPIVERSIONEX = LPAPI_VERSION(WINAPI*)(LPAPI_VERSION AppVersion);
    using PFNMINIDUMPWRITEDUMP = BOOL(WINAPI *)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile,
        MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
        CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam,
        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

    static BOOL CALLBACK MiniDumpCallback(PVOID param, PMINIDUMP_CALLBACK_INPUT input,
                                          PMINIDUMP_CALLBACK_OUTPUT output);
    void processMinidump(PMINIDUMP_CALLBACK_INPUT input, PMINIDUMP_CALLBACK_OUTPUT output);

    HMODULE dbgHelpHandle_;
    PFNIMAGEHLPAPIVERSIONEX fnImagehlpApiVersionEx_;
    PFNMINIDUMPWRITEDUMP fnMiniDumpWriteDump_;
};

CrashDumpInternal::CrashDumpInternal()
{
    dbgHelpHandle_ = ::LoadLibraryEx(L"dbghelp.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (dbgHelpHandle_) {
        fnImagehlpApiVersionEx_ = reinterpret_cast<PFNIMAGEHLPAPIVERSIONEX>(
            GetProcAddress(dbgHelpHandle_, "ImagehlpApiVersionEx"));
        fnMiniDumpWriteDump_ = reinterpret_cast<PFNMINIDUMPWRITEDUMP>(
            GetProcAddress(dbgHelpHandle_, "MiniDumpWriteDump"));
    } else {
        CRASH_LOG_ERROR("Failed to load dbghelp.dll");
        fnImagehlpApiVersionEx_ = nullptr;
        fnMiniDumpWriteDump_ = nullptr;
    }
}

CrashDumpInternal::~CrashDumpInternal()
{
    if (dbgHelpHandle_)
        ::FreeLibrary(dbgHelpHandle_);
}

bool CrashDumpInternal::isAvailable() const
{
    return fnMiniDumpWriteDump_ != nullptr;
}

void CrashDumpInternal::setApiVersion() const
{
    if (!fnImagehlpApiVersionEx_)
        return;

    API_VERSION requiredApiVersion;
    requiredApiVersion.MajorVersion = 6;
    requiredApiVersion.MinorVersion = 1;
    requiredApiVersion.Revision = 11;
    requiredApiVersion.Reserved = 0;

    auto *api_version = fnImagehlpApiVersionEx_(&requiredApiVersion);
    char kActualApiString[64] = { 0 };
    std::sprintf(kActualApiString, "%i.%i.%i",
        api_version->MajorVersion, api_version->MinorVersion, api_version->Revision);
    if (requiredApiVersion.MajorVersion > api_version->MajorVersion ||
        (requiredApiVersion.MajorVersion == api_version->MajorVersion &&
         requiredApiVersion.MinorVersion > api_version->MinorVersion) ||
            (requiredApiVersion.MajorVersion == api_version->MajorVersion &&
             requiredApiVersion.MinorVersion == api_version->MinorVersion &&
             requiredApiVersion.Revision > api_version->Revision)) {
        char kRequiredApiString[64] = { 0 };
        std::sprintf(kRequiredApiString, "%i.%i.%i",
            requiredApiVersion.MajorVersion, requiredApiVersion.MinorVersion,
            requiredApiVersion.Revision);
        CRASH_LOG_ERROR("Failed to set DbgHelp API version ({} should be {})",
                  kActualApiString, kRequiredApiString);
    } else {
        CRASH_LOG_INFO("DbgHelp API version is {}", kActualApiString);
    }
}

void CrashDumpInternal::setPrivileges() const
{
    HANDLE handle = 0;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &handle)) {
        CRASH_LOG_ERROR("Failed to set dump privileges (can't get process token)");
        return;
    }
    TOKEN_PRIVILEGES tokenPrivileges;
    tokenPrivileges.PrivilegeCount = 1;
    LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tokenPrivileges.Privileges[0].Luid);
    tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(
        handle, FALSE, &tokenPrivileges, sizeof(tokenPrivileges), nullptr, nullptr)) {
        CRASH_LOG_ERROR("Failed to set dump privileges (can't adjust token privileges)");
    }
    CloseHandle(handle);
}

bool CrashDumpInternal::writeMinidump(const std::wstring &filename, DWORD thread_id,
                                      PEXCEPTION_POINTERS exception_pointers)
{
    const auto file_handle = CreateFile(filename.c_str(), GENERIC_WRITE, 0, nullptr,
                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        CRASH_LOG_ERROR(L"Failed to open minidump file for writing: {}", filename);
        return false;
    }

    const auto process_id = GetCurrentProcessId();
    const auto process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);

    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = thread_id;
    mei.ExceptionPointers = exception_pointers;
    mei.ClientPointers = TRUE;

    MINIDUMP_CALLBACK_INFORMATION mci;
    mci.CallbackRoutine = MiniDumpCallback;
    mci.CallbackParam = this;

    bool success = fnMiniDumpWriteDump_(process_handle, process_id, file_handle,
        MiniDumpWithIndirectlyReferencedMemory, &mei, nullptr, &mci);

    CloseHandle(process_handle);
    CloseHandle(file_handle);
    return success;
}

void CrashDumpInternal::processMinidump(PMINIDUMP_CALLBACK_INPUT input,
                                        PMINIDUMP_CALLBACK_OUTPUT /*output*/)
{
    switch (input->CallbackType) {
    case ModuleCallback:
        CRASH_LOG_INFO(L"Dumping info for module {}", input->Module.FullPath);
        break;
    case ThreadCallback:
        CRASH_LOG_INFO("Dumping info for thread {}", input->Thread.ThreadId);
        break;
    default:
        break;
    }
}

// static
BOOL CALLBACK CrashDumpInternal::MiniDumpCallback(PVOID param, PMINIDUMP_CALLBACK_INPUT input,
                                                  PMINIDUMP_CALLBACK_OUTPUT output)
{
    auto *this_ = static_cast<CrashDumpInternal*>(param);
    if (this_)
        this_->processMinidump(input, output);
    return TRUE;
}

CrashDump::CrashDump() : internal_(new CrashDumpInternal)
{
}

CrashDump::~CrashDump() = default;

bool CrashDump::writeToFile(const std::wstring &filename, unsigned int thread_handle,
                            void *exception_pointers)
{
    if (!internal_->isAvailable())
        return false;

    internal_->setPrivileges();
    internal_->setApiVersion();
    return internal_->writeMinidump(
        filename, thread_handle, static_cast<PEXCEPTION_POINTERS>(exception_pointers));
}

}

#endif  // ENABLE_CRASH_REPORTS
