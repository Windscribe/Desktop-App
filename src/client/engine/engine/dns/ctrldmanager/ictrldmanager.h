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

    // If user supplies DoH resolver that's on *.controld.com, append ?int=ws to the URI when making queries.
    // ie. user spplies: https://dns.controld.com/abcd12344 -> send queries to https://dns.controld.com/abcd12344?int=ws
    QString addWsSuffix(const QString &upstream)
    {
        if (upstream.contains("https://", Qt::CaseInsensitive) && upstream.contains("controld.com", Qt::CaseInsensitive))
            return upstream + "?int=ws";
        else
            return upstream;
    }
};


