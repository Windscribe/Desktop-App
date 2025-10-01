#pragma once

class SplitTunnelServiceManager
{
public:
    SplitTunnelServiceManager();

    bool start() const;
    void stop() const;
};

