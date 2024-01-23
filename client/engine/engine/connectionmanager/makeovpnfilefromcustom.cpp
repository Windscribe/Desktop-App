#include "makeovpnfilefromcustom.h"

#include "engine/customconfigs/parseovpnconfigline.h"

#if defined (Q_OS_WIN)
#include "types/global_consts.h"
#endif

MakeOVPNFileFromCustom::MakeOVPNFileFromCustom() : config_()
{
}

MakeOVPNFileFromCustom::~MakeOVPNFileFromCustom()
{
}

// write all of ovpnData to file and add remoteCommand with replaced ip
bool MakeOVPNFileFromCustom::generate(const QString &customConfigPath, const QString &ovpnData, const QString &ip, const QString &remoteCommand)
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

#if defined (Q_OS_WIN)
    // We use the --dev-node option to ensure OpenVPN will only use the dco/wintun adapter instance we create and not
    // possibly attempt to use an adapter created by other software (e.g. the vanilla OpenVPN client app).
    config_ += QString("\r\n--dev-node %1\r\n").arg(kOpenVPNAdapterIdentifier);
    config_ += "\r\n--windows-driver wintun\r\n";
#endif

    return true;
}
