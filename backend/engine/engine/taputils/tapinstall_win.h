#ifndef TAPINSTALL_WIN_H
#define TAPINSTALL_WIN_H

#include <QString>
#include <windows.h>

class TapInstall_win
{
public:
    enum TAP_TYPE { TAP_ADAPTER, WIN_TUN};

    static bool installTap(const QString &subfolder);
    static bool reinstallTap(TAP_TYPE tap);
    static QVector<TAP_TYPE> detectInstalledTapDriver(bool bWithLog);

private:
    static std::wstring NTPathToDosPath(LPCWSTR ntPath);
    static bool reinstallTapWithFolders(const QString &installSubfolder, const QString &uninstallSubfolder);
};

#endif // TAPINSTALL_WIN_H
