#ifndef PINGSTORAGE_H
#define PINGSTORAGE_H

#include "engine/types/locationid.h"

#include <QObject>
#include <QHash>
#include <QMutex>
#include "pingtime.h"

namespace locationsmodel {

// stores information about pings, persistent between runs of the program
class PingStorage
{
public:
    explicit PingStorage();
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
    };

    QHash<QString, PingData> hash_;
    quint32 curIteration_;

    void saveToSettings();
    void loadFromSettings();

};

} //namespace locationsmodel

#endif // PINGSTORAGE_H
