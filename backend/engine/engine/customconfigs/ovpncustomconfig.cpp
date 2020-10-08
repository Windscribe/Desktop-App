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
    config->isCorrect_ = true;      // by default correct config
    config->process();              // here the config can change to incorrect
    return config;
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
                remotes_ << rcl;
                qDebug(LOG_CUSTOM_OVPN) << "Extracted hostname/IP:" << openVpnLine.host << " from  remote cmd:" << line;
                bFoundAtLeastOneRemote = true;
            }
            else
            {
                ovpnData_ += line + "\n";
            }
        }

        if (!bFoundAtLeastOneRemote)
        {
            qDebug(LOG_CUSTOM_OVPN) << "Ovpn config file" << filepath_ << "incorrect, because can't find remote host command. The file format may be incorrect.";
            isCorrect_ = false;
            errMessage_ = "Can't find remote host command";
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

