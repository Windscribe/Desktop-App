#pragma once

#include <QString>
#include <QList>

namespace MacUtils
{
    void activateApp();
    void invalidateShadow(void *pNSView);
    void invalidateCursorRects(void *pNSView);
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    bool isOsVersionAtLeast(int major, int minor);
    QString getOsVersion();
    QString getBundlePath();

    void hideDockIcon();
    void showDockIcon();
    void setHandCursor();
    void setArrowCursor();

    bool isLockdownMode();
    QSet<QString> getOsDnsServers();

    // CLI
    bool isAppAlreadyRunning();
    bool showGui();

    // Split Routing
    QString iconPathFromBinPath(const QString &binPath);
    QList<QString> enumerateInstalledPrograms();

    void getNSWindowCenter(void *nsView, int &outX, int &outY);
    bool dynamicStoreEntryHasKey(const QString &entry, const QString &key);
    bool verifyAppBundleIntegrity();

    bool isParentProcessGui();
}
