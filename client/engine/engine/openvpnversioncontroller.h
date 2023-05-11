#pragma once

#include <QStringList>

//thread safe
class OpenVpnVersionController
{
public:
    static OpenVpnVersionController &instance()
    {
        static OpenVpnVersionController s;
        return s;
    }

    QString getOpenVpnVersion();

    // Returns an absolute path to the OpenVPN binary, including the file name.
    QString getOpenVpnFilePath();

    // Return the file name of the OpenVPN binary, excluding the path.
    QString getOpenVpnFileName();

    void setUseWinTun(bool bUseWinTun);
    bool isUseWinTun();

private:
    OpenVpnVersionController();

    bool bUseWinTun_ = false;
    QString ovpnVersion_;

    void detectVersion();
};
