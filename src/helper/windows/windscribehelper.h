#pragma once

#include "fwpm_wrapper.h"
#include "win32service.h"
#include "utils/win32handle.h"

namespace wsl
{

class WindscribeHelper : public Win32Service
{
public:
    WindscribeHelper();
    ~WindscribeHelper() override;

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

private:
    bool createClientPipe();
    void processClientRequests();
    void sendRequestResult(HANDLE hIOEvent, const std::string &data);
    void stop();

    static DWORD WINAPI clientHandlerThreadProc(LPVOID lpParameter);
};

} // end namespace wsl
