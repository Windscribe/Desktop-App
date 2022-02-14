#ifndef LAUNCHONSTARTUP_H
#define LAUNCHONSTARTUP_H


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

#endif // LAUNCHONSTARTUP_H
