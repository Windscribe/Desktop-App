#pragma once

#include <chrono>
#include <list>
#include <string>
#include <mutex>

//---------------------------------------------------------------------------
// We keep an in-memory list of log entries until writeFile is called.  This
// ensures no hackery (e.g. symbolic linking the log file to a system file)
// can be performed while the installer is preparing the the target folder.
//

class Log
{
public:
    static Log& instance()
    {
        static Log log;
        return log;
    }

    void init(bool installing);
    void out(const char* format, ...);
    void out(const wchar_t* format, ...);
    void out(const std::wstring& message);
    void writeFile(const std::wstring& installPath) const;

    static void WSDebugMessage(const wchar_t* format, ...);

private:
    bool installing_ = false;
    std::recursive_mutex mutex_;
    std::list<std::wstring> logEntries_;
    std::chrono::time_point<std::chrono::system_clock> start_;

    Log();
    ~Log();

    void writeToSystemDebugger() const;
};
