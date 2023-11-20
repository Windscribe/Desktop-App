#pragma once

#include <QList>
#include <QObject>
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/ping/pingmultiplehosts.h"

// It's actually a manual test
class PingMultiplePings_test : public QObject
{
    Q_OBJECT

public:
    PingMultiplePings_test();

    QList<QString> testIps() const;
    void setTestIps(const QList<QString> &newTestIps);

signals:
    void testIpsChanged();

private slots:
    void testBasic();

private:
    NetworkAccessManager *accessManager_;

    QList< QPair<QString, QString>>  testIps_;
    QList< QPair<QString, QString>> testFailedIps_;

    PingType getNextPingType(PingType pt)
    {
        if (pt == PingType::kTcp) return PingType::kIcmp;
        else if (pt == PingType::kIcmp) return PingType::kCurl;
        else if (pt == PingType::kCurl) return PingType::kTcp;
        else return PingType::kCurl;
    }

    QList<QPair<QString, QString>> readIps(const QString &filename);
};





