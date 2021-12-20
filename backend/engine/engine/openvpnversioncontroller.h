#ifndef OPENVPNVERSIONCONTROLLER_H
#define OPENVPNVERSIONCONTROLLER_H

#include <QStringList>
#include <QMutex>

//thread safe
class OpenVpnVersionController
{
public:
    static OpenVpnVersionController &instance()
    {
        static OpenVpnVersionController s;
        return s;
    }

    QStringList getAvailableOpenVpnVersions();
    QStringList getAvailableOpenVpnExecutables();
    QString getSelectedOpenVpnVersion();
    QString getSelectedOpenVpnExecutable();

    void setSelectedOpenVpnVersion(const QString &version);

    void setUseWinTun(bool bUseWinTun);
    bool isUseWinTun();

private:
    OpenVpnVersionController();

    QStringList openVpnVersionsList_;
    QStringList openVpnFilesList_;
    int selectedInd_;
    QMutex mutex_;
    bool bUseWinTun_;

    QString getOpenVpnBinaryPath();
    QString detectVersion(const QString &path);
};

#endif // OPENVPNVERSIONCONTROLLER_H
