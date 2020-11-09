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

    path_ = QStandardPaths::writableLocation(QStandardPaths::DataLocation)
          + "/windscribe_temp_config.ovpn";
    file_.setFileName(path_);

    if (!file_.open(QIODevice::WriteOnly))
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

    file_.flush();
    return true;
}

