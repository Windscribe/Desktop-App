#pragma once

#include <QString>

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

private:
    OpenVpnVersionController();

    QString ovpnVersion_;

    void detectVersion();
};
