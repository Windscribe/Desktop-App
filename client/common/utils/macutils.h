#pragma once

#include <QString>
#include <QList>

namespace MacUtils
{
    void activateApp();
    void invalidateShadow(void *pNSView);
    void invalidateCursorRects(void *pNSView);
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    QString getOsVersion();
    QString getBundlePath();

    bool isOsVersion10_11_or_greater();
    bool isOsVersionIsSierra_or_greater();
    bool isOsVersionIsCatalina_or_greater();
    bool isOsVersionIsBigSur_or_greater();

    void hideDockIcon();
    void showDockIcon();
    void setHandCursor();
    void setArrowCursor();

    // CLI
    bool isGuiAlreadyRunning();
    bool showGui();

    // Split Routing
    QString iconPathFromBinPath(const QString &binPath);
    QList<QString> enumerateInstalledPrograms();

    void getNSWindowCenter(void *nsView, int &outX, int &outY);
    bool dynamicStoreEntryHasKey(const QString &entry, const QString &key);
    bool verifyAppBundleIntegrity();

    bool isParentProcessGui();
}




