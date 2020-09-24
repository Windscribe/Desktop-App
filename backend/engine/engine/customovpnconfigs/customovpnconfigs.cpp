#include "customovpnconfigs.h"
#include <QDir>
#include <QStandardPaths>
#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "parseovpnconfigline.h"

CustomOvpnConfigs::CustomOvpnConfigs(QObject *parent) : QObject(parent), dirWatcher_(NULL)
{
}

void CustomOvpnConfigs::changeDir(const QString &path)
{
    if (dirWatcher_)
    {
        delete dirWatcher_;
    }

    if (!path.isEmpty())
    {
        dirWatcher_ = new CustomConfigsDirWatcher(this, path);
        connect(dirWatcher_, SIGNAL(dirChanged()), SLOT(onDirectoryChanged()));
        parseDir();
        emit changed();
    }
}

/*QSharedPointer<ServerLocation> CustomOvpnConfigs::getLocation()
{
    return serverLocation_;
}
*/
bool CustomOvpnConfigs::isExist()
{
    //return !serverLocation_.isNull();
    return false;
}

void CustomOvpnConfigs::onDirectoryChanged()
{
    qDebug(LOG_CUSTOM_OVPN) << "custom_configs directory is changed";
    parseDir();
    emit changed();
}

void CustomOvpnConfigs::parseDir()
{
#if 0
    QDir dir(dirWatcher_->curDir());
    QStringList filters;
    filters << "*.ovpn";
    dir.setNameFilters(filters);
    QStringList fileList = dir.entryList(QDir::Files);

    QVector<ServerNode> nodes;
    Q_FOREACH(const QString &filename, fileList)
    {
        // skip our temp file
        if (filename == "windscribe_temp_config.ovpn")
        {
            continue;
        }
        QString pathFile = dir.path() + "/" + filename;
        QSharedPointer<ServerNode> node = makeServerNodeFromOvpnFile(pathFile);
        if (!node.isNull())
        {
            nodes << *node;
        }
    }

    serverLocation_.reset();
    if (nodes.count() > 0)
    {
        serverLocation_ = QSharedPointer<ServerLocation>(new ServerLocation);
        serverLocation_->transformToCustomOvpnLocation(nodes);
    }
#endif
}

/*
QSharedPointer<ServerNode> CustomOvpnConfigs::makeServerNodeFromOvpnFile(const QString &pathFile)
{
    QFile file(pathFile);
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
#if 0
            else if (openVpnLine.type == ParseOvpnConfigLine::OVPN_CMD_AUTH_USER_PASS) // auth-user-pass "path"
            {
                customOvpnAuthCredentialsStorage_->setAuthCredentials(pathFile, openVpnLine.username, openVpnLine.password);
            }
#endif
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
    }
}
*/
