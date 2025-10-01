#pragma once

class LaunchOnStartup_win
{
public:
    static void setLaunchOnStartup(bool enable);

private:
    static void clearDisableAutoStartFlag();

};
