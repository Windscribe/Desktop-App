#ifndef LAUNCHONSTARTUP_WIN_H
#define LAUNCHONSTARTUP_WIN_H

class LaunchOnStartup_win
{
public:
    static void setLaunchOnStartup(bool enable);

private:
    static void clearDisableAutoStartFlag();

};

#endif // LAUNCHONSTARTUP_WIN_H
