#pragma once

#include <string>

#include "fwpm_wrapper.h"
#include "win32service.h"
#include "utils/win32handle.h"

namespace wsl
{

class Helper : public Win32Service
{
public:
    Helper();
    ~Helper() override;

protected:
    bool onInit() override;
    void runService() override;
    void onUserControl(DWORD controlCode) override;
    void onStopRequest() override;
    void onWindowsShutdown() override;

private:
    Win32Handle clientHandlerThread_;
    Win32Handle clientPipe_;
    FwpmWrapper fwpmWrapper_;

    // Must ensure this struct outlives the life of any overlapped IO call made in readAll/writeAll.
    OVERLAPPED overlapped_;

    // Holds the request/response buffers for the IPC transaction currently in flight.  Like
    // overlapped_, these must outlive any overlapped IO call in readAll/writeAll: when a stop
    // abandons a pending read or write via the WAIT_IO_COMPLETION path, the kernel may still
    // complete that IO against these buffers, so they cannot live on the stack of
    // processClientRequests.
    struct ClientMessage
    {
        int cmdId = 0;
        unsigned long requestSize = 0;
        std::string request;
        unsigned long responseSize = 0;
        std::string response;
    };
    ClientMessage clientMessage_;

private:
    bool createClientPipe();
    void processClientRequests();
    void stop();

    bool readAll(HANDLE hIOEvent, char *buf, DWORD len);
    bool writeAll(HANDLE hIOEvent, const char *buf, DWORD len);

    static DWORD WINAPI clientHandlerThreadProc(LPVOID lpParameter);
};

} // end namespace wsl
