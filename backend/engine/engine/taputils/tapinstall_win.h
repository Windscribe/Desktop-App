#ifndef TAPINSTALL_WIN_H
#define TAPINSTALL_WIN_H

#include <QString>
#include <windows.h>

class TapInstall_win
{
public:
    enum TAP_TYPE { TAP_ADAPTER, WIN_TUN};

    static QVector<TAP_TYPE> detectInstalledTapDriver(bool bWithLog);
};

#endif // TAPINSTALL_WIN_H
