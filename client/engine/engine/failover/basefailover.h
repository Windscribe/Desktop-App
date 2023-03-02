#pragma once

#include <QDateTime>
#include <QObject>

class NetworkAccessManager;

namespace failover {

class FailoverData {

public:
    FailoverData(const QString &domain) : domain_(domain) {}
    FailoverData(const QString &domain, const QString &echConfig, QDateTime ttlDateExpire) :
        domain_(domain), echConfig_(echConfig), ttlDateExpire_(ttlDateExpire) {}

    QString domain() const { return domain_; }
    QString echConfig() const { return echConfig_; }
    QDateTime ttlDateExpire() const { return ttlDateExpire_; }

    // only for debug purpose
    friend QDebug operator<<(QDebug dbg, const FailoverData &d) {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{domain:" << d.domain_ << "; ";
        dbg << "ech:" << d.echConfig_ << "; ";
        dbg << "ttl:" << d.ttlDateExpire_ << "}";
        return dbg;
    }

private:
    QString   domain_;
    QString   echConfig_;     // empty if it does not support ECH
    QDateTime ttlDateExpire_;
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
    virtual bool isEch() const { return false; }        // must return true for ECH failover

signals:
    void finished(const QVector<failover::FailoverData> &data);     // if data is empty then it is implied that failed

protected:
     NetworkAccessManager *networkAccessManager_;
     QString uniqueId_;
};


} // namespace failover

