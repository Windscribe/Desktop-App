#pragma once

#include <QObject>

// Managing the launch of the external ctrld utility
class ICtrldManager : public QObject
{
    Q_OBJECT

public:
    explicit ICtrldManager(QObject *parent, bool isCreateLog): QObject(parent), isCreateLog_(isCreateLog) {}
    virtual ~ICtrldManager() {}

    virtual bool runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains) = 0;
    virtual void killProcess() = 0;
    virtual QString listenIp() const = 0;

protected:
    bool isCreateLog_;
};


