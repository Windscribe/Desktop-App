#ifndef UNIQUEIPLIST_H
#define UNIQUEIPLIST_H

#include <QString>
#include <QSet>

class UniqueIpList
{
public:
    void add(const QString &ip);
    QSet<QString> get() const;

private:
    QSet<QString> set_;
};

#endif // UNIQUEIPLIST_H
