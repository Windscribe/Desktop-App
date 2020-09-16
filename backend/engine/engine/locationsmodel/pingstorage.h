#ifndef PINGSTORAGE_H
#define PINGSTORAGE_H

#include "engine/types/locationid.h"

#include <QObject>
#include <QHash>
#include <QMutex>
#include "engine/serversmodel/pingtime.h"

namespace locationsmodel {

// stores information about pings, persistent between runs of the program
class PingStorage
{
public:
    explicit PingStorage();
    virtual ~PingStorage();

    void updateNodes(const QStringList &ips);
    void setNodePing(const QString &nodeIp, PingTime timeMs);
    PingTime getNodeSpeed(const QString &nodeIp) const;

    quint32 getCurrentIteration() const;
    void incIteration();

    /*QSet<QString> getNodesWithoutPingData(const QStringList &ips);

    void saveToSettings();
    void loadFromSettings();*/

private:

    /*struct PingData
    {
        bool isUsed_;
        PingTime timeMs_;
        quint32 iteration_;
    };

    QHash<QString, PingData> hash_;*/
    quint32 curIteration_;

    /*QMutex mutex_;

    bool isAllPingsHaveSameIteration(quint32 &outIteration);*/
};

} //namespace locationsmodel

#endif // PINGSTORAGE_H
