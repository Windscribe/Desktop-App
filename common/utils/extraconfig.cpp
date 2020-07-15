#include "extraconfig.h"

#include <QFile>
#include <QStandardPaths>
#include "../Utils/logger.h"

void ExtraConfig::writeConfig(const QString &cfg)
{
    QMutexLocker locker(&mutex_);
    QFile file(path_);
    if (cfg.isEmpty())
    {
        file.remove();
    }
    else
    {
        if (file.open(QIODevice::WriteOnly))
        {
            file.resize(0);
            file.write(cfg.toLocal8Bit());
            file.close();
        }
    }
}


QString ExtraConfig::getExtraConfig(bool bWithLog)
{
    QMutexLocker locker(&mutex_);
    QFile fileExtra(path_);
    if (fileExtra.open(QIODevice::ReadOnly))
    {
        QByteArray extraArr = fileExtra.readAll();
        if (bWithLog)
        {
            qCDebug(LOG_BASIC) << "Found extra config file:" << path_;
            qCDebug(LOG_BASIC) << "Extra options:" << extraArr;
        }

        fileExtra.close();
        return extraArr;
    }
    else
    {
        return "";
    }
}

QString ExtraConfig::getExtraConfigForOpenVpn()
{
    QMutexLocker locker(&mutex_);
    QString result;
    QString strExtraConfig = getExtraConfig();
    QStringList strs = strExtraConfig.split("\n");
    Q_FOREACH(const QString &line, strs)
    {
        QString lineTrimmed = line.trimmed();
        if (!lineTrimmed.startsWith("--ikev2", Qt::CaseInsensitive))
        {
            result += line + "\n";
        }
    }
    return result;
}

QString ExtraConfig::getExtraConfigForIkev2()
{
    QMutexLocker locker(&mutex_);
    QString result;
    QString strExtraConfig = getExtraConfig();
    QStringList strs = strExtraConfig.split("\n");
    Q_FOREACH(const QString &line, strs)
    {
        QString lineTrimmed = line.trimmed();
        if (lineTrimmed.startsWith("--ikev2", Qt::CaseInsensitive))
        {
            result += line + "\n";
        }
    }
    return result;
}

bool ExtraConfig::isUseIkev2Compression()
{
    QMutexLocker locker(&mutex_);
    QString config = getExtraConfigForIkev2();
    return config.contains("--ikev2-compression", Qt::CaseInsensitive);
}

QString ExtraConfig::getRemoteIpFromExtraConfig()
{
    QMutexLocker locker(&mutex_);
    QString strExtraConfig = getExtraConfig(false);
    QStringList strs = strExtraConfig.split("\n");
    Q_FOREACH(const QString &line, strs)
    {
        if (line.contains("remote", Qt::CaseInsensitive))
        {
            QStringList words = line.split(" ");
            if (words.size() == 2)
            {
                if (words[0].trimmed().compare("remote", Qt::CaseInsensitive) == 0)
                {
                    return words[1].trimmed();
                }
            }
        }
    }
    return "";
}

QByteArray ExtraConfig::modifyVerbParameter(const QByteArray &ovpnData, QString &strExtraConfig)
{
    QRegularExpressionMatch match;
    int indExtra = strExtraConfig.indexOf(regExp_, 0, &match);
    if (indExtra == -1)
    {
        return ovpnData;
    }
    QString verbString = strExtraConfig.mid(indExtra, match.capturedLength());

    QString strOvpn = ovpnData;

    int ind = strOvpn.indexOf(regExp_, 0, &match);
    if (ind == -1)
    {
        return ovpnData;
    }

    strOvpn.replace(regExp_, verbString);
    strExtraConfig.remove(indExtra, match.capturedLength());

    return strOvpn.toLocal8Bit();
}

ExtraConfig::ExtraConfig(): mutex_(QMutex::Recursive), regExp_("(?m)^(?i)(verb)(\\s+)(\\d+$)")
{
    path_ = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/windscribe_extra.conf";
}
