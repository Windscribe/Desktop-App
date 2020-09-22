#ifndef CUSTOMOVPNCONFIGS_H
#define CUSTOMOVPNCONFIGS_H

#include <QObject>
//#include "engine/apiinfo/serverlocation.h"
#include "customconfigsdirwatcher.h"

// parse custom_configs directory, make ovpn configs location, resolve IP for hostnames in ovpn-files
class CustomOvpnConfigs : public QObject
{
    Q_OBJECT
public:
    explicit CustomOvpnConfigs(QObject *parent);

    void changeDir(const QString &path);

    //QSharedPointer<ServerLocation> getLocation();
    bool isExist();

signals:
    void changed();

private slots:
    void onDirectoryChanged();

private:
    void parseDir();

    //QSharedPointer<ServerLocation> serverLocation_;
    CustomConfigsDirWatcher *dirWatcher_;

    // return NULL pointer if failed
    //QSharedPointer<ServerNode> makeServerNodeFromOvpnFile(const QString &pathFile);

};

#endif // CUSTOMOVPNCONFIGS_H
