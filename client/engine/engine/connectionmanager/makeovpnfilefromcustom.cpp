#include "makeovpnfilefromcustom.h"
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include "utils/extraconfig.h"
#include "utils/logger.h"
#include "engine/customconfigs/parseovpnconfigline.h"

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
    #include "engine/tempscripts_mac.h"
#endif

MakeOVPNFileFromCustom::MakeOVPNFileFromCustom()
{
    path_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
          + "/windscribe_temp_config.ovpn";
    file_.setFileName(path_);
}

MakeOVPNFileFromCustom::~MakeOVPNFileFromCustom()
{
    file_.close();
    file_.remove();
}

// write all of ovpnData to file and add remoteCommand with replaced ip
bool MakeOVPNFileFromCustom::generate(const QString &customConfigPath, const QString &ovpnData, const QString &ip, const QString &remoteCommand)
{
    if (file_.isOpen())
    {
        file_.close();
        file_.remove();
    }

    if (!file_.open(QIODeviceBase::WriteOnly))
    {
        qCDebug(LOG_CONNECTION) << "Can't open config file:" << file_.fileName();
        return false;
    }

    file_.resize(0);

    QString customConfigPathCopy(customConfigPath);
    QString cd_command = QString("cd \"%1\"\n\n").arg(customConfigPathCopy.replace("\\", "/"));
    file_.write(cd_command.toLocal8Bit());

    file_.write(ovpnData.toLocal8Bit());

    QString line = remoteCommand;
    ParseOvpnConfigLine::OpenVpnLine openVpnLine = ParseOvpnConfigLine::processLine(remoteCommand);
    if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_REMOTE_IP)
    {
        line = line.replace(openVpnLine.host, ip);
        file_.write(line.toLocal8Bit());
        file_.write("\r\n");
    }

#ifdef Q_OS_MAC
    // No need to set "script-security" here, because it is handled in the config parsing code.
    QString strDnsPath = TempScripts_mac::instance().dnsScriptPath();
    if (strDnsPath.isEmpty()) {
        return false;
    }
    QString cmd1 = "\nup \"" + strDnsPath + " -up\"\n";
    file_.write(cmd1.toUtf8());
#endif

    file_.flush();
    return true;
}

