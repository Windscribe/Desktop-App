#pragma once
#include <QHash>
#include <QObject>
#include <QSet>

class WhitelistIpsManager : public QObject
{
    Q_OBJECT

public:
    WhitelistIpsManager(QObject *parent);

    void add(const QString &hostname, const QStringList &ips);
    void remove(const QString &hostname);

signals:
    void whitelistIpsChanged(const QSet<QString> &ips);

private:
    QHash<QString, QStringList> hash_;
    QSet<QString> lastWhitelistIps_;

    void updateIps();
};

