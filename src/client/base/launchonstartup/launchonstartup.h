#pragma once

class LaunchOnStartup
{
public:

    static LaunchOnStartup &instance()
    {
        static LaunchOnStartup l;
        return l;
    }

    void setLaunchOnStartup(bool enable);

private:
    LaunchOnStartup() {}
};
