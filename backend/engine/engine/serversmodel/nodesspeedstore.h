#ifndef NODESSPEEDSTORE_H
#define NODESSPEEDSTORE_H

#include "engine/types/locationid.h"

#include <QObject>
#include <QHash>
#include <QMutex>
#include "pingtime.h"

class NodesSpeedStore : public QObject
{
    Q_OBJECT
public:
    explicit NodesSpeedStore(QObject *parent);
    virtual ~NodesSpeedStore();

    void updateNodes(const QStringList &nodes);
    void setNodeSpeed(const QString &nodeIp, PingTime timeMs, quint32 iteration);
    PingTime getNodeSpeed(const QString &nodeIp);

    QSet<QString> getNodesWithoutPingData(const QStringList &ips);

    void saveToSettings();
    void loadFromSettings();

signals:
    void pingIterationChanged();

private:

    struct PingData
    {
        bool isUsed_;
        PingTime timeMs_;
        quint32 iteration_;
    };

    QHash<QString, PingData> hash_;
    quint32 curIteration_;

    QMutex mutex_;

    bool isAllPingsHaveSameIteration(quint32 &outIteration);
};

#endif // NODESSPEEDSTORE_H
