#ifndef CUSTOMCONFIGLOCATIONINFO_H
#define CUSTOMCONFIGLOCATIONINFO_H

#include <QHostInfo>
#include <QSharedPointer>
#include "baselocationinfo.h"
#include "engine/customconfigs/icustomconfig.h"


namespace locationsmodel {

// describe consig location
class CustomConfigLocationInfo : public BaseLocationInfo
{
    Q_OBJECT
public:
    explicit CustomConfigLocationInfo(const LocationID &locationId, QSharedPointer<const customconfigs::ICustomConfig> config);

    bool isExistSelectedNode() const override;
    void resolveHostnames();

    QString getSelectedIp() const;
    QString getOvpnData() const;
    QString getFilename() const;

signals:
    void hostnamesResolved();

private slots:
    void onResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer);

private:
    struct RemoteDescr
    {
        QString ipOrHostname_;
        bool isHostname;
        bool isResolved;
        QStringList ipsForHostname_;
    };

    QSharedPointer<const customconfigs::ICustomConfig> config_;
    QVector<RemoteDescr> remotes_;

    bool isAllResolved() const;

};

} //namespace locationsmodel

#endif // CUSTOMCONFIGLOCATIONINFO_H
