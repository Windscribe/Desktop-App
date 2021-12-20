#ifndef PINGSTORAGE_H
#define PINGSTORAGE_H

#include <QObject>
#include <QHash>
#include <QMutex>
#include "pingtime.h"

namespace locationsmodel {

// stores information about pings, persistent between runs of the program
class PingStorage
{
public:
    explicit PingStorage(const QString &settingsKeyName);
    virtual ~PingStorage();

    void updateNodes(const QStringList &ips);

    void setNodePing(const QString &nodeIp, PingTime timeMs, bool fromDisconnectedState);
    PingTime getNodeSpeed(const QString &nodeIp) const;

    quint32 getCurrentIteration() const;
    void incIteration();

    void getState(bool &isAllNodesHaveCurIteration, bool &isAllNodesInDisconnectedState);

private:

    struct PingData
    {
        PingTime timeMs_;
        quint32 iteration_;
        bool fromDisconnectedState_;

        PingData() : iteration_(0), fromDisconnectedState_(false) {}
    };

    QHash<QString, PingData> hash_;
    quint32 curIteration_;

    QString settingsKeyName_;

    void saveToSettings();
    void loadFromSettings();

};

} //namespace locationsmodel

#endif // PINGSTORAGE_H
