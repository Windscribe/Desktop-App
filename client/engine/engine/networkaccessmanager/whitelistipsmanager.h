#pragma once
#include <QHash>
#include <QObject>
#include <QSet>

class WhitelistIpsManager : public QObject
{
    Q_OBJECT

public:
    WhitelistIpsManager(QObject *parent);

    void add(const QStringList &ips);
    void remove(const QStringList &ips);

signals:
    void whitelistIpsChanged(const QSet<QString> &ips);

private:
    QSet<QString> ips_;
};

