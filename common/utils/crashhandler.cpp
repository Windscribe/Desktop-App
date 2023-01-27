#include "crashhandler.h"
#include "crashdump.h"

#if defined(ENABLE_CRASH_REPORTS)

#include <ctime>
#include <intrin.h>
#include <new.h>
#include <signal.h>
#include <Windows.h>

#if defined(WINDSCRIBE_SERVICE)
#include "../../backend/windows/windscribe_service/logger.h"
#else
#include "logger.h"
#include <QStandardPaths>
#endif

namespace Debug {

struct ProcessExceptionHandlers
{
    using SignalHandlerFun = void(__cdecl *)(int);
    SignalHandlerFun sigAbrtHandler = nullptr;
    SignalHandlerFun sigIntHandler = nullptr;
    SignalHandlerFun sigTermHandler = nullptr;
    LPTOP_LEVEL_EXCEPTION_FILTER  seHandler = nullptr;
    _purecall_handler pureCallHandler = nullptr;
    _invalid_parameter_handler ipHandler = nullptr;
    _PNH newHandler = nullptr;
};

struct ThreadExceptionHandlers
{
    using SignalHandlerFun = void(__cdecl *)(int);
    SignalHandlerFun sigSegvHandler = nullptr;
    SignalHandlerFun sigIllHandler = nullptr;
    terminate_handler termHandler;
    unexpected_handler unexpectedHandler;
};

struct StackOverflowThreadInfo
{
    explicit StackOverflowThreadInfo(PEXCEPTION_POINTERS exception_pointers,
                                     unsigned int current_thread_id) :
        exceptionPointers(exception_pointers), crashedThreadId(current_thread_id) {}

    PEXCEPTION_POINTERS exceptionPointers;
    unsigned int crashedThreadId;
};

enum class ExceptionType { SEH, TERMINATE, UNEXPECTED, PURECALL, NEW_OPERATOR, INVALID_PARAMETER,
                           SIG_ABRT, SIG_ILL, SIG_INT, SIG_SEGV, SIG_TERM, NUM_EXCEPTION_TYPES };

const char *kExceptionTypeNames[static_cast<int>(ExceptionType::NUM_EXCEPTION_TYPES)] = {
    "SEH", "TERMINATE", "UNEXPECTED", "PURECALL", "NEW_OPERATOR", "INVALID_PARAMETER",
    "SIGNAL_ABRT", "SIGNAL_ILL", "SIGNAL_INT", "SIGNAL_SEGV", "SIGNAL_TERM"
};

struct ExceptionInfo
{
    explicit ExceptionInfo(ExceptionType type, void *exception_pointers = nullptr,
                           unsigned int current_thread_id = 0) :
        exceptionType(type),
        exceptionPointers(static_cast<PEXCEPTION_POINTERS>(exception_pointers)),
        exceptionThreadId(current_thread_id ? current_thread_id : GetCurrentThreadId()),
        expression(nullptr), function(nullptr), file(nullptr), line(0) { }

    PEXCEPTION_POINTERS exceptionPointers;
    ExceptionType exceptionType;
    unsigned int exceptionThreadId;
    const wchar_t* expression;
    const wchar_t* function;
    const wchar_t* file;
    unsigned int line;
};

namespace
{
unsigned int GetCurrentThreadId()
{
    return ::GetCurrentThreadId();
}

std::wstring GetOutputLocation()
{
#if defined(WINDSCRIBE_SERVICE)
    wchar_t buffer[MAX_PATH] = { 0 };
    GetModuleFileName(0, buffer, sizeof(buffer));
    std::wstring strBuffer{ buffer };
    strBuffer = strBuffer.substr(0, strBuffer.find_last_of(L"\\/"));
    return strBuffer + TEXT("/");
#else
    return QString("%1/").arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                         .toStdWString();
#endif
}

LONG WINAPI LocalSehHandler(__in PEXCEPTION_POINTERS exceptionPointers)
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::SEH, exceptionPointers));
    return EXCEPTION_EXECUTE_HANDLER;
}

DWORD WINAPI StackOverflowThreadHandler(LPVOID threadParameter)
{
    auto *exceptionInfo = static_cast<StackOverflowThreadInfo*>(threadParameter);
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::SEH, 
                                                           exceptionInfo->exceptionPointers,
                                                           exceptionInfo->crashedThreadId));
    return 0;
}

void SignalHandler_Abrt(int)
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::SIG_ABRT, _pxcptinfoptrs));
}

void SignalHandler_Ill(int)
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::SIG_ILL, _pxcptinfoptrs));
}

void SignalHandler_Int(int)
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::SIG_INT, _pxcptinfoptrs));
}

void SignalHandler_Segv(int)
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::SIG_SEGV, _pxcptinfoptrs));
}

void SignalHandler_Term(int)
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::SIG_TERM, _pxcptinfoptrs));
}

void __cdecl InvalidParameterHandler(const wchar_t* expression, const wchar_t* function,
                                     const wchar_t* file, unsigned int line, uintptr_t /*reserved*/)
{
    ExceptionInfo ei(ExceptionType::INVALID_PARAMETER);
    ei.expression = expression;
    ei.function = function;
    ei.file = file;
    ei.line = line;
    CrashHandler::instance().handleException(std::move(ei));
}

void __cdecl PureCallHandler()
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::PURECALL));
}

int __cdecl NewHandler(size_t)
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::NEW_OPERATOR));
    return 0;
}

void __cdecl TerminateHandler()
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::TERMINATE));
}

void __cdecl UnexpectedHandler()
{
    CrashHandler::instance().handleException(ExceptionInfo(ExceptionType::UNEXPECTED));
}
}

std::wstring CrashHandler::moduleName_(L"unknown");

CrashHandler::CrashHandler()
{
    typedef BOOL(WINAPI * SETPROCESSUSERMODEEXCEPTIONPOLICY)(DWORD dwFlags);
    typedef BOOL(WINAPI * GETPROCESSUSERMODEEXCEPTIONPOLICY)(LPDWORD lpFlags);
    const DWORD kProcessCallbackFilterEnabled = 1u;

    auto hKernel32 = LoadLibrary(TEXT("kernel32.dll"));
    if (hKernel32) {
        SETPROCESSUSERMODEEXCEPTIONPOLICY pfnSetProcessUserModeExceptionPolicy =
            (SETPROCESSUSERMODEEXCEPTIONPOLICY)GetProcAddress(
                hKernel32, "SetProcessUserModeExceptionPolicy");
        GETPROCESSUSERMODEEXCEPTIONPOLICY pfnGetProcessUserModeExceptionPolicy =
            (GETPROCESSUSERMODEEXCEPTIONPOLICY)GetProcAddress(
                hKernel32, "GetProcessUserModeExceptionPolicy");
        if (pfnSetProcessUserModeExceptionPolicy && pfnGetProcessUserModeExceptionPolicy) {
            DWORD dwFlags = 0;
            if (pfnGetProcessUserModeExceptionPolicy(&dwFlags))
                pfnSetProcessUserModeExceptionPolicy(dwFlags & ~kProcessCallbackFilterEnabled);
        }
        FreeLibrary(hKernel32);
    }
}

CrashHandler::~CrashHandler()
{
    unbindFromThread();
    unbindFromProcess();
}

bool CrashHandler::bindToProcess()
{
    CRASH_ASSERT(!oldProcessHandlers_);
    if (oldProcessHandlers_) {
        CRASH_LOG("CrashHandler::bindToProcess: tried to install process handlers twice");
        return false;
    }

    // Reset old exception handlers.
    oldProcessHandlers_.reset(new ProcessExceptionHandlers);

    // Set new handlers.
    oldProcessHandlers_->seHandler = SetUnhandledExceptionFilter(LocalSehHandler);
    oldProcessHandlers_->sigAbrtHandler = signal(SIGABRT, SignalHandler_Abrt);
    oldProcessHandlers_->sigIntHandler = signal(SIGINT, SignalHandler_Int);
    oldProcessHandlers_->sigTermHandler = signal(SIGTERM, SignalHandler_Term);
    oldProcessHandlers_->ipHandler = _set_invalid_parameter_handler(InvalidParameterHandler);
    oldProcessHandlers_->pureCallHandler = _set_purecall_handler(PureCallHandler);
    oldProcessHandlers_->newHandler = _set_new_handler(NewHandler);

    // Install for the main thread as well.
    return bindToThread();
}

void CrashHandler::unbindFromProcess()
{
    if (!oldProcessHandlers_)
        return;

    // Restore old exception handlers.
    const auto *oh = oldProcessHandlers_.get();
    if (oh) {
        if (oh->seHandler)
            SetUnhandledExceptionFilter(oh->seHandler);
        if (oh->sigAbrtHandler)
            signal(SIGABRT, oh->sigAbrtHandler);
        if (oh->sigIntHandler)
            signal(SIGINT, oh->sigIntHandler);
        if (oh->sigTermHandler)
            signal(SIGTERM, oh->sigTermHandler);
        if (oh->ipHandler)
            _set_invalid_parameter_handler(oh->ipHandler);
        if (oh->pureCallHandler)
            _set_purecall_handler(oh->pureCallHandler);
        if (oh->newHandler)
            _set_new_handler(oh->newHandler);
    }
}

bool CrashHandler::bindToThread()
{
    const auto thread_id = GetCurrentThreadId();

    std::lock_guard<std::mutex> locker(threadHandlerMutex_);
    auto it = oldThreadHandlers_.find(thread_id);
    if (it != oldThreadHandlers_.end()) {
        CRASH_LOG("CrashHandler::bindToThread: tried to install handlers twice - thread 0x%08x",
                  thread_id);
        return false;
    }
    auto *thread_handlers = new ThreadExceptionHandlers;
    thread_handlers->sigIllHandler = signal(SIGILL, SignalHandler_Ill);
    thread_handlers->sigSegvHandler = signal(SIGSEGV, SignalHandler_Segv);
    thread_handlers->termHandler = std::set_terminate(TerminateHandler);
    thread_handlers->unexpectedHandler = std::set_unexpected(UnexpectedHandler);
    oldThreadHandlers_.emplace(thread_id, thread_handlers);
    return true;
}

void CrashHandler::unbindFromThread()
{
    const auto thread_id = GetCurrentThreadId();

    std::lock_guard<std::mutex> locker(threadHandlerMutex_);
    auto it = oldThreadHandlers_.find(thread_id);
    if (it == oldThreadHandlers_.end())
        return;

    // Restore old exception handlers.
    const auto *oh = it->second.get();
    if (oh) {
        if (oh->sigIllHandler)
            signal(SIGILL, oh->sigIllHandler);
        if (oh->sigSegvHandler)
            signal(SIGSEGV, oh->sigSegvHandler);
        if (oh->termHandler)
            std::set_terminate(oh->termHandler);
        if (oh->unexpectedHandler)
            std::set_unexpected(oh->unexpectedHandler);
    }
    oldThreadHandlers_.erase(it);
}

void CrashHandler::handleException(ExceptionInfo info)
{
    auto currentThreadId = GetCurrentThreadId();

    // SEH stack overflow exception is a special case, because we need some stack space to run
    // the minidump creation code. Do this in a separate thread.
    if (info.exceptionType == ExceptionType::SEH && info.exceptionPointers &&
        info.exceptionPointers->ExceptionRecord &&
        info.exceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW &&
        info.exceptionThreadId == currentThreadId) {
        StackOverflowThreadInfo soti(info.exceptionPointers, currentThreadId);
        auto thread = CreateThread(nullptr, 0, &StackOverflowThreadHandler, &soti, 0, nullptr);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        TerminateProcess(GetCurrentProcess(), 1);
        return;
    }

    std::lock_guard<std::mutex> locker(crashMutex_);
    CRASH_LOG("------ CRASH in %ls ------", moduleName_.c_str());

    EXCEPTION_RECORD exceptionRecord;
    CONTEXT contextRecord;
    EXCEPTION_POINTERS exceptionPointers;
    exceptionPointers.ExceptionRecord = &exceptionRecord;
    exceptionPointers.ContextRecord = &contextRecord;

    if (!info.exceptionPointers) {
        getExceptionPointers(&exceptionPointers);
        info.exceptionPointers = &exceptionPointers;
    }

    CRASH_LOG(" type = %s", kExceptionTypeNames[static_cast<int>(info.exceptionType)]);
    if (info.exceptionPointers) {
        CRASH_LOG(" code = 0x%08x", info.exceptionPointers->ExceptionRecord->ExceptionCode);
        CRASH_LOG(" addr = 0x%08x", info.exceptionPointers->ExceptionRecord->ExceptionAddress);
    }
    if (info.expression)
        CRASH_LOG(" expression = %ls", info.expression);
    if (info.function)
        CRASH_LOG(" function = %ls", info.function);
    if (info.file)
        CRASH_LOG(" file = %ls@%u", info.file, info.line);

    CrashDump minidump;
    const std::wstring filename = getCrashDumpFilename();
    if (minidump.writeToFile(GetOutputLocation() + filename, info.exceptionThreadId,
                             info.exceptionPointers))
        CRASH_LOG("Wrote minidump: %ls", filename.c_str());

    TerminateProcess(GetCurrentProcess(), 1);
}

void CrashHandler::getExceptionPointers(void *exceptionPointers) const
{
    CONTEXT contextRecord;
    memset(&contextRecord, 0, sizeof(CONTEXT));

#if defined(_M_IX86)
    __asm {
        mov dword ptr[contextRecord.Eax], eax
        mov dword ptr[contextRecord.Ecx], ecx
        mov dword ptr[contextRecord.Edx], edx
        mov dword ptr[contextRecord.Ebx], ebx
        mov dword ptr[contextRecord.Esi], esi
        mov dword ptr[contextRecord.Edi], edi
        mov word ptr[contextRecord.SegSs], ss
        mov word ptr[contextRecord.SegCs], cs
        mov word ptr[contextRecord.SegDs], ds
        mov word ptr[contextRecord.SegEs], es
        mov word ptr[contextRecord.SegFs], fs
        mov word ptr[contextRecord.SegGs], gs
        pushfd
        pop[contextRecord.EFlags]
    }
    contextRecord.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable:4311)
    contextRecord.Eip = (ULONG)_ReturnAddress();
    contextRecord.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
    contextRecord.Ebp = *((ULONG *)_AddressOfReturnAddress() - 1);
#elif defined (_M_X64)
    // Need to fill up the Context in IA64 and AMD64.
    RtlCaptureContext(&contextRecord);
#endif

    auto *output = static_cast<EXCEPTION_POINTERS*>(exceptionPointers);
    memcpy(output->ContextRecord, &contextRecord, sizeof(CONTEXT));
    memset(output->ExceptionRecord, 0, sizeof(EXCEPTION_RECORD));
    output->ExceptionRecord->ExceptionAddress = _ReturnAddress();
}

std::wstring CrashHandler::getCrashDumpFilename() const
{
    const time_t now = time(0);
    const struct tm time_struct = *localtime(&now);
    wchar_t time_buffer[128] = { 0 };
    std::wcsftime(time_buffer, sizeof(time_buffer) / sizeof(time_buffer[0]),
                  TEXT("%Y-%m-%d_%H-%M-%S"), &time_struct);
    return moduleName_ + TEXT("_") + std::wstring(time_buffer) + TEXT(".dmp");
}

void CrashHandler::crashForTest() const
{
    // This code intentionally produces a crash, to test crashdump functionality.
    volatile unsigned int *bad_pointer = nullptr;
    // cppcheck-suppress nullPointer
    *bad_pointer = 0xdeadfa11;
}

#pragma warning(disable: 4717)
void CrashHandler::stackOverflowForTest() const
{
    struct temp_stack_allocation_for_test {
        ~temp_stack_allocation_for_test() { ++i[0]; }
        volatile unsigned long long i[10000];
    } temp;  // cppcheck-suppress unusedVariable
    stackOverflowForTest();
}
#pragma warning(default: 4717)

}

#endif  // ENABLE_CRASH_REPORTS
