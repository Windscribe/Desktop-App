#ifndef PARSEOVPNCONFIGLINE_H
#define PARSEOVPNCONFIGLINE_H

#include <QStringList>

class ParseOvpnConfigLine
{
public:

    enum OvpnCmd { OVPN_CMD_UNKNOWN, OVPN_CMD_REMOTE_IP, OVPN_CMD_AUTH_USER_PASS, OVPN_CMD_CA};

    struct OpenVpnLine
    {
        OvpnCmd type; // OVPN_CMD_UNKNOWN - unknown, OVPN_CMD_REMOTE_IP - remote ip,
                      // OVPN_CMD_AUTH_USER_PASS - auth-user-pass "path"

        // if both is empty, then need ask
        QString username;
        QString password;

        QString host;
        QString caPath_;

        uint port;
    };

    static OpenVpnLine processLine(const QString &line);

private:
    static QStringList splitLine(const QString &line);
};

#endif // PARSEOVPNCONFIGLINE_H
