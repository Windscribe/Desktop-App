#include "customconfigs.h"
#include <QDir>
#include "utils/log/categories.h"
#include "parseovpnconfigline.h"
#include "ovpncustomconfig.h"
#include "wireguardcustomconfig.h"

namespace customconfigs {

CustomConfigs::CustomConfigs(QObject *parent) : QObject(parent), dirWatcher_(NULL)
{
}

void CustomConfigs::changeDir(const QString &path)
{
    if (dirWatcher_)
    {
        if (dirWatcher_->curDir() == path)
        {
            return;
        }
        delete dirWatcher_;
        dirWatcher_ = nullptr;
    }
    else if (path.isEmpty())
    {
        return;
    }

    if (!path.isEmpty())
    {
        dirWatcher_ = new CustomConfigsDirWatcher(this, path);
        connect(dirWatcher_, &CustomConfigsDirWatcher::dirChanged, this, &CustomConfigs::onDirectoryChanged);
    }
    parseDir();
    emit changed();
}

QVector<QSharedPointer<const ICustomConfig> > CustomConfigs::getConfigs()
{
    return configs_;
}

void CustomConfigs::onDirectoryChanged()
{
    qDebug(LOG_CUSTOM_OVPN) << "custom_configs directory is changed";
    parseDir();
    emit changed();
}

void CustomConfigs::parseDir()
{
    configs_.clear();

    if (!dirWatcher_)
        return;

    QStringList fileList = dirWatcher_->curFiles();
    for (const QString &filename : fileList)
    {
        QString filepath = dirWatcher_->curDir() + "/" + filename;
        QSharedPointer<const ICustomConfig> config = makeCustomConfigFromFile(filepath);
        if (!config.isNull())
        {
            configs_ << config;
        }
    }
}

QSharedPointer<const ICustomConfig> CustomConfigs::makeCustomConfigFromFile(const QString &filepath)
{
    QFileInfo fi(filepath);
    QString fileSuffix = fi.suffix();
    if (fileSuffix.compare("ovpn", Qt::CaseInsensitive) == 0)
    {
        return QSharedPointer<const ICustomConfig>(OvpnCustomConfig::makeFromFile(filepath));
    }
    else if (fileSuffix.compare("conf", Qt::CaseInsensitive) == 0)
    {
        return QSharedPointer<const ICustomConfig>(WireguardCustomConfig::makeFromFile(filepath));
    }

    return NULL;
}

} //namespace customconfigs
