#pragma once

#include <QDateTime>
#include <QElapsedTimer>
#include <QObject>
#include "utils/ws_assert.h"

class NetworkAccessManager;

namespace failover {

class FailoverData {

public:
    explicit FailoverData(const QString &domain) : domain_(domain) {}
    explicit FailoverData(const QString &domain, const QString &echConfig, int ttl) :
        domain_(domain), echConfig_(echConfig), ttl_(ttl)
    {
        elapsedTimer_.start();
    }

    QString domain() const { return domain_; }
    QString echConfig() const { return echConfig_; }
    bool isExpired() const
    {
        WS_ASSERT(!echConfig_.isEmpty());
        return elapsedTimer_.hasExpired(ttl_ * 1000);
    }

    // only for debug purpose
    friend QDebug operator<<(QDebug dbg, const FailoverData &d) {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{domain:" << d.domain_ << "; ";
        dbg << "ech:" << d.echConfig_ << "; ";
        dbg << "ttl:" << d.ttl_ << "}";
        return dbg;
    }

private:
    QString   domain_;
    QString   echConfig_;     // empty if it does not support ECH
    int ttl_;                 // TTL in seconds
    QElapsedTimer elapsedTimer_;
};

class BaseFailover : public QObject
{
    Q_OBJECT
public:
    explicit BaseFailover(QObject *parent, const QString &uniqueId, NetworkAccessManager *networkAccessManager = nullptr) :
        uniqueId_(uniqueId), QObject(parent), networkAccessManager_(networkAccessManager)
    {
    }

    virtual void getData(bool bIgnoreSslErrors) = 0;
    virtual QString name() const = 0;
    QString uniqueId() const { return uniqueId_; }
signals:
    void finished(const QVector<failover::FailoverData> &data);     // if data is empty then it is implied that failed

protected:
     NetworkAccessManager *networkAccessManager_;
     QString uniqueId_;
};


} // namespace failover

