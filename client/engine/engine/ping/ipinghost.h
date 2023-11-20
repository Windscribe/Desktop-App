#pragma once

#include <QObject>

class NetworkAccessManager;

class IPingHost : public QObject
{
    Q_OBJECT

public:
    explicit IPingHost(QObject *parent) : QObject(parent) {}

    virtual void ping() = 0;

signals:
    void pingFinished(bool success, int timems, const QString &ip);
};
