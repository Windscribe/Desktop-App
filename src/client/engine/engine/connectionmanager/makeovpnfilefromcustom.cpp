#include "makeovpnfilefromcustom.h"

#include "engine/customconfigs/parseovpnconfigline.h"

MakeOVPNFileFromCustom::MakeOVPNFileFromCustom() : config_()
{
}

MakeOVPNFileFromCustom::~MakeOVPNFileFromCustom()
{
}

// write all of ovpnData to file and add remoteCommand with replaced ip
bool MakeOVPNFileFromCustom::generate(const QString &customConfigPath, const QString &ovpnData, const QString &ip, const QString &remoteCommand, const QString &customDns)
{
    config_ = "";

    QString customConfigPathCopy(customConfigPath);
    QString cd_command = QString("cd \"%1\"\n\n").arg(customConfigPathCopy.replace("\\", "/"));
    config_ += cd_command;
    config_ += ovpnData;

    QString line = remoteCommand;
    ParseOvpnConfigLine::OpenVpnLine openVpnLine = ParseOvpnConfigLine::processLine(remoteCommand);
    if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_REMOTE_IP) {
        line = line.replace(openVpnLine.host, ip);
        config_ += line + "\r\n";
    }

    if (!customDns.isEmpty()) {
        config_ += "\r\npull-filter ignore \"dhcp-option DNS\"\r\n";
        config_ += QString("dhcp-option DNS %1\r\n").arg(customDns);
    }

    return true;
}
