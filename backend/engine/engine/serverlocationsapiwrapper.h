#ifndef SERVERLOCATIONSAPIWRAPPER_H
#define SERVERLOCATIONSAPIWRAPPER_H

#include <QObject>
#include <QDataStream>
#include <QHash>
#include "serversmodel/nodesspeedstore.h"
#include "ping/pinghost.h"
#include "serverapi/serverapi.h"

// describe detected BestLocation
class BestLocationInfo
{
public:
    BestLocationInfo()
    {
        isValid_ = false;
    }

    void set(int id, QString selectedHostName, int pingTimeMs)
    {
        id_ = id;
        selectedHostName_ = selectedHostName;
        pingTimeMs_ = pingTimeMs;
        isValid_ = true;
    }

    void readFromSettings()
    {
        QSettings settings;
        if (settings.contains("bestLocation"))
        {
            QByteArray arr = settings.value("bestLocation").toByteArray();
            QDataStream stream(&arr, QIODevice::ReadOnly);
            int revisionNumber;
            stream >> revisionNumber;
            if (revisionNumber != BEST_LOCATION_SAVED_REVISION_NUMBER)
            {
                isValid_ = false;
                return;
            }
            stream >> id_;
            stream >> selectedHostName_;
            stream >> pingTimeMs_;
            isValid_ = true;
        }
        else
        {
            isValid_ = false;
        }
    }
    void writeToSettings()
    {
        QSettings settings;
        if (isValid_)
        {
            QByteArray arr;
            {
                QDataStream stream(&arr, QIODevice::WriteOnly);
                stream << BEST_LOCATION_SAVED_REVISION_NUMBER;
                stream << id_;
                stream << selectedHostName_;
                stream << pingTimeMs_;
            }
            settings.setValue("bestLocation", arr);
        }
        else
        {
            settings.remove("bestLocation");
        }
    }

    bool isValid() const
    {
        return isValid_;
    }

    int getId() const
    {
        Q_ASSERT(isValid_);
        return id_;
    }

    QString getSelectedHostname() const
    {
        Q_ASSERT(isValid_);
        return selectedHostName_;
    }

    int getPingMs() const
    {
        Q_ASSERT(isValid_);
        return pingTimeMs_;
    }

private:
    bool isValid_;
    int id_;   // id, on which can be found in ServerLocations list
    QString selectedHostName_;
    int pingTimeMs_;
    const int BEST_LOCATION_SAVED_REVISION_NUMBER = 2;
};

// wrapper for serverLocations API call
// its purpose is to determine BestLocation after calling the serverLocations API and return servers locations list with best location
// the class can also generate a signal by itself updatedBestLocation, when new ping data is available and best location was changed
// this call use NodesSpeedRating for detect
class ServerLocationsApiWrapper : public QObject
{
    Q_OBJECT
public:
    explicit ServerLocationsApiWrapper(QObject *parent, NodesSpeedStore *nodesSpeedStore, ServerAPI *serverAPI);
    virtual ~ServerLocationsApiWrapper();

    void serverLocations(const QString &authHash, const QString &language, uint userRole, bool isNeedCheckRequestsEnabled,
                         const QString &revision, bool isPro, ProtocolType protocol, const QStringList &alcList);

signals:
    void serverLocationsAnswer(SERVER_API_RET_CODE retCode, QVector< QSharedPointer<ServerLocation> > serverLocations,
                               QStringList forceDisconnectNodes, uint userRole);

    void updatedBestLocation(QVector< QSharedPointer<ServerLocation> > &serverLocations);

    void updateFirewallIpsForLocations(QVector<QSharedPointer<ServerLocation> > &serverLocations);

private slots:
    void onServerLocationsAnswer(SERVER_API_RET_CODE retCode, QVector< QSharedPointer<ServerLocation> > serverLocations, QStringList forceDisconnectNodes, uint userRole);
    void onPingIterationChanged();
    void onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);


private:
    NodesSpeedStore *nodesSpeedStore_;
    ServerAPI *serverAPI_;
    BestLocationInfo bestLocation_;

    QVector<QSharedPointer<ServerLocation> > serverLocations_;
    QStringList forceDisconnectNodes_;
    uint userRole_;

    PingHost *pingHost_;
    QHash<QString, int> pings_;

    bool detectBestLocation(QVector<QSharedPointer<ServerLocation> > &serverLocations, int &outInd, int &outSelNodeInd, int &outTimeMs);
    // return false, if no data available for all nodes
    bool averageLatencyForLocation(ServerLocation *sl, int &outAverageLatency, int &outMinLatency, int &outIndNodeWithMinLatency);

    bool isAllPingsFinished();
    bool isNeedChangeBestLocation(int selectedNodeInd, PingTime pingTimeMs);
    QSharedPointer<ServerLocation> createBestLocation();

    void selectRandomBestLocation();

    void outputBestLocationToLog(int ind, int selectedNodeInd, PingTime pingTimeMs);
};

#endif // SERVERLOCATIONSAPIWRAPPER_H
