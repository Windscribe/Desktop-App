#ifndef PARSEOVPNCONFIGLINE_H
#define PARSEOVPNCONFIGLINE_H

#include <QStringList>

class ParseOvpnConfigLine
{
public:

    enum OvpnCmd { OVPN_CMD_UNKNOWN, OVPN_CMD_REMOTE_IP, OVPN_CMD_PROTO, OVPN_CMD_PORT};

    struct OpenVpnLine
    {
        OvpnCmd type;
        QString host;   // extracted hostname/ip from "remote" cmd

        uint port;          // 0 if not set
        QString protocol;   // empty if not set

        OpenVpnLine() : type(OVPN_CMD_UNKNOWN), port(0) {}

    };

    static OpenVpnLine processLine(const QString &line);

private:
    static QStringList splitLine(const QString &line);
};

#endif // PARSEOVPNCONFIGLINE_H
