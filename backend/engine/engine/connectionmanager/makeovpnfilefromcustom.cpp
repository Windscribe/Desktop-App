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
    QString strPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/custom_configs";
    QDir dir(strPath);
    dir.mkpath(strPath);
    path_ = strPath + "/windscribe_temp_config.ovpn";
    port_ = 0;
    file_.setFileName(path_);
}

MakeOVPNFileFromCustom::~MakeOVPNFileFromCustom()
{
    file_.close();
    file_.remove();
}

// if IP is not empty, need replace all hostnames in remote command to this IP
bool MakeOVPNFileFromCustom::generate(const QString &pathCustomOvpn, const QString &ip)
{
    if (!file_.isOpen())
    {
        if (!file_.open(QIODevice::WriteOnly))
        {
            qCDebug(LOG_CONNECTION) << "Can't open config file:" << file_.fileName();
            return false;
        }
    }

    file_.resize(0);

    QFile customOvpnFile(pathCustomOvpn);
    if (!customOvpnFile.open(QIODevice::ReadOnly))
    {
        qCDebug(LOG_CONNECTION) << "Can't open custom ovpn config file:" << pathCustomOvpn;
        return false;
    }

    QTextStream in(&customOvpnFile);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        ParseOvpnConfigLine::OpenVpnLine openVpnLine = ParseOvpnConfigLine::processLine(line);

        if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_REMOTE_IP)
        {
            line = line.replace(openVpnLine.host, ip);
            file_.write(line.toLocal8Bit());
            file_.write("\r\n");
            port_ = openVpnLine.port;
        }
        else
        {
            file_.write(line.toLocal8Bit());
            file_.write("\r\n");
        }
    }

    file_.flush();

    return true;
}
