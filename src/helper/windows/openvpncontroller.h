#pragma once

#include "executecmd.h"
#include "utils/win32handle.h"

class OpenVPNController final
{
public:
    static OpenVPNController &instance()
    {
        static OpenVPNController wc;
        return wc;
    }

    void release();

    bool createAdapter(bool useDCODriver);
    void removeAdapter();

    // OpenVPN now chooses its own management port (the config requests "management 127.0.0.1 0").
    // On success, outPort is set to the OS-assigned management port resolved from the spawned
    // process, and the returned ExecuteCmdResult::processId carries the OpenVPN PID. The engine
    // uses these to verify it connects to the genuine, helper-spawned OpenVPN.
    ExecuteCmdResult runOpenvpn(std::wstring &config, const std::wstring &httpProxy, unsigned int httpPort,
                                const std::wstring &socksProxy, unsigned int socksPort, unsigned int &outPort);

private:
    explicit OpenVPNController();

    bool useDCODriver_ = false;
    bool tapAdapterCreated_ = false;

    // Kept open for the lifetime of the OpenVPN process so its PID cannot be reused while it runs.
    wsl::Win32Handle openVpnProcess_;

    bool createDCOAdapter();
    void removeDCOAdapter();

    bool createTapAdapter();
    void removeTapAdapter();

    bool writeOVPNFile(std::wstring &config, const std::wstring &httpProxy, unsigned int httpPort,
                       const std::wstring &socksProxy, unsigned int socksPort, std::wstring &filename);

    // Polls the TCP table for the loopback management socket OpenVPN (pid) is listening on.
    bool resolveManagementPort(unsigned long pid, unsigned int &outPort);
};
