#ifndef UNIQUEIPLIST_H
#define UNIQUEIPLIST_H

#include <QString>
#include <QSet>

class UniqueIpList
{
public:
    void add(const QString &ip);
    QString getFirewallString();

private:
    QSet<QString> set_;
};

#endif // UNIQUEIPLIST_H
