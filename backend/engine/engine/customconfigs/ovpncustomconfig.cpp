#include "ovpncustomconfig.h"
#include "utils/logger.h"
#include "parseovpnconfigline.h"

#include <QFileInfo>

namespace customconfigs {

CUSTOM_CONFIG_TYPE OvpnCustomConfig::type() const
{
    return CUSTOM_CONFIG_OPENVPN;
}

QString OvpnCustomConfig::name() const
{
    return name_;
}

QString OvpnCustomConfig::nick() const
{
    return nick_;
}

QString OvpnCustomConfig::filename() const
{
    return filename_;
}

QStringList OvpnCustomConfig::hostnames() const
{
    QStringList list;
    for (auto r : remotes_)
    {
        list << r.hostname;
    }
    return list;
}

bool OvpnCustomConfig::isAllowFirewallAfterConnection() const
{
    return isAllowFirewallAfterConnection_;
}

bool OvpnCustomConfig::isCorrect() const
{
    return isCorrect_;
}

QString OvpnCustomConfig::getErrorForIncorrect() const
{
    return errMessage_;
}

ICustomConfig *OvpnCustomConfig::makeFromFile(const QString &filepath)
{
    OvpnCustomConfig *config = new OvpnCustomConfig();
    QFileInfo fi(filepath);
    config->name_ = fi.completeBaseName();
    config->filename_ = fi.fileName();
    config->filepath_ = filepath;
    config->globalPort_ = 0;
    config->isCorrect_ = true;      // by default correct config
    config->process();              // here the config can change to incorrect
    return config;
}

QVector<RemoteCommandLine> OvpnCustomConfig::remotes() const
{
    return remotes_;
}

uint OvpnCustomConfig::globalPort() const
{
    return globalPort_;
}

QString OvpnCustomConfig::globalProtocol() const
{
    return globalProtocol_;
}

QString OvpnCustomConfig::getOvpnData() const
{
    return ovpnData_;
}

// retrieves all hostnames/IPs "remote ..." commands
// also ovpn-config with removed "remote" commands saves in ovpnData_
// removed "remote" commands are saved in remotes_ (to restore the config with changed hostnames/IPs before connect)
void OvpnCustomConfig::process()
{
    QFile file(filepath_);
    if (file.open(QIODevice::ReadOnly))
    {
        qDebug(LOG_CUSTOM_OVPN) << "Opened:" << filepath_;

        bool bFoundAtLeastOneRemote = false;
        bool bFoundVerbCommand = false;
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            ParseOvpnConfigLine::OpenVpnLine openVpnLine = ParseOvpnConfigLine::processLine(line);
            if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_REMOTE_IP) // remote ip
            {
                RemoteCommandLine rcl;
                rcl.hostname = openVpnLine.host;
                rcl.originalRemoteCommand = line;
                rcl.port = openVpnLine.port;
                rcl.protocol = openVpnLine.protocol;
                remotes_ << rcl;
                qDebug(LOG_CUSTOM_OVPN) << "Extracted hostname/IP:" << openVpnLine.host << " from  remote cmd:" << line;
                if (!openVpnLine.protocol.isEmpty())
                {
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted protocol:" << openVpnLine.protocol << " from  remote cmd:" << line;
                }
                if (openVpnLine.port != 0)
                {
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted port:" << openVpnLine.port << " from  remote cmd:" << line;
                }

                bFoundAtLeastOneRemote = true;
            }
            else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_VERB) // verb cmd
            {
                qDebug(LOG_CUSTOM_OVPN) << "Extracted verb:" << openVpnLine.verb;
                if (openVpnLine.verb < 3)
                    openVpnLine.verb = 3;

                ovpnData_ += "verb " + QString::number(openVpnLine.verb) + "\n";
                bFoundVerbCommand = true;
            }
            else
            {
                if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_PROTO) // proto cmd
                {
                    globalProtocol_ = openVpnLine.protocol;
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted global protocol:" << globalProtocol_;
                }
                else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_PORT) // port cmd
                {
                    globalPort_ = openVpnLine.port;
                    qDebug(LOG_CUSTOM_OVPN) << "Extracted global port:" << globalPort_;
                }
                else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_IGNORE_REDIRECT_GATEWAY)
                {
                    isAllowFirewallAfterConnection_ = false;
                    qDebug(LOG_CUSTOM_OVPN)
                        << "Extracted information: ignore redirect-gateway (" << line << ")";
                }

                ovpnData_ += line + "\n";
            }
        }

        if (!bFoundVerbCommand)
            ovpnData_ += "verb 3\n";

        if (!bFoundAtLeastOneRemote)
        {
            qDebug(LOG_CUSTOM_OVPN) << "Ovpn config file" << filepath_ << "incorrect, because can't find remote host command. The file format may be incorrect.";
            isCorrect_ = false;
            errMessage_ = "Can't find remote host command";
        }
        else
        {
            nick_ = remotes_[0].hostname;
        }
    }
    else
    {
        qDebug(LOG_CUSTOM_OVPN) << "Failed to open file" << filepath_;
        isCorrect_ = false;
        errMessage_ = "Failed to open file";
    }
}


} //namespace customconfigs

