#pragma once

class SplitTunnelServiceManager
{
public:
    SplitTunnelServiceManager();

    bool start();
    void stop();

private:
    // True only after a stop this process issued succeeded. The driver is demand-start and only
    // this class starts it, so it stays stopped until start() runs. Starts false, so the first
    // stop after a helper (re)start always reaches the SCM.
    bool isKnownStopped_ = false;
};
