#include "ovpncustomconfig.h"

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
    return QStringList();
}

bool OvpnCustomConfig::isCorrect() const
{
    return true;
}

QString OvpnCustomConfig::getErrorForIncorrect() const
{
    return "";
}

ICustomConfig *OvpnCustomConfig::makeFromFile(const QString &filepath)
{
    OvpnCustomConfig *config = new OvpnCustomConfig();
    QFileInfo fi(filepath);
    config->name_ = fi.completeBaseName();
    config->filename_ = fi.fileName();
    config->filepath_ = filepath;
    config->process();
    return config;
}

void OvpnCustomConfig::process()
{
    /*QFile file(pathFile);
    if (file.open(QIODevice::ReadOnly))
    {
        qDebug(LOG_CUSTOM_OVPN) << "Opened" << pathFile;

        bool isAlreadyWasHostname = false;
        QString hostname;

        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            ParseOvpnConfigLine::OpenVpnLine openVpnLine = ParseOvpnConfigLine::processLine(line);
            if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_REMOTE_IP) // remote ip
            {
                if (isAlreadyWasHostname)
                {
                    qDebug(LOG_CUSTOM_OVPN) << "Ovpn config file" << pathFile << "skipped, because it has multiple remote host commands. This is not supported now.";
                    return QSharedPointer<ServerNode>();
                }
                else
                {
                    hostname = openVpnLine.host;
                    isAlreadyWasHostname = true;
                }
            }
            else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_AUTH_USER_PASS) // auth-user-pass "path"
            {
                customOvpnAuthCredentialsStorage_->setAuthCredentials(pathFile, openVpnLine.username, openVpnLine.password);
            }
        }

        if (!hostname.isEmpty())
        {
            QSharedPointer<ServerNode> node(new ServerNode);
            node->initFromCustomOvpnConfig(QFileInfo(pathFile).completeBaseName(), hostname, pathFile);
            return node;
        }
        else
        {
            qDebug(LOG_CUSTOM_OVPN) << "Ovpn config file" << pathFile << "skipped, because can't find remote host command. The file format may be incorrect.";
            return QSharedPointer<ServerNode>();
        }
    }
    else
    {
        qDebug(LOG_CUSTOM_OVPN) << "Failed to open file" << pathFile;
        return QSharedPointer<ServerNode>();
    }*/
}


} //namespace customconfigs

