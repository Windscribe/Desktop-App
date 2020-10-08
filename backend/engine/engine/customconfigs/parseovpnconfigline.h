#ifndef PARSEOVPNCONFIGLINE_H
#define PARSEOVPNCONFIGLINE_H

#include <QStringList>

class ParseOvpnConfigLine
{
public:

    enum OvpnCmd { OVPN_CMD_UNKNOWN, OVPN_CMD_REMOTE_IP};

    struct OpenVpnLine
    {
        OvpnCmd type;
        QString host;   // extracted hostname/ip from "remote" cmd


        OpenVpnLine() : type(OVPN_CMD_UNKNOWN) {}

    };

    static OpenVpnLine processLine(const QString &line);

private:
    static QStringList splitLine(const QString &line);
};

#endif // PARSEOVPNCONFIGLINE_H
