#ifndef CUSTOMCONFIGS_H
#define CUSTOMCONFIGS_H

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include "icustomconfig.h"
#include "customconfigsdirwatcher.h"

namespace customconfigs {

// parse custom configs directory, make ovpn configs location
class CustomConfigs : public QObject
{
    Q_OBJECT
public:
    explicit CustomConfigs(QObject *parent);

    void changeDir(const QString &path);
    QVector<QSharedPointer<const ICustomConfig>> getConfigs();

signals:
    void changed();

private slots:
    void onDirectoryChanged();

private:
    void parseDir();

    QSharedPointer<const ICustomConfig> makeCustomConfigFromFile(const QString &filepath);

    CustomConfigsDirWatcher *dirWatcher_;
    QVector<QSharedPointer<const ICustomConfig>> configs_;
};

} //namespace customconfigs

#endif // CUSTOMCONFIGS_H
