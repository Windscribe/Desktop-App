#ifndef PERSISTENTSTATE_H
#define PERSISTENTSTATE_H

#include "ipc/generated_proto/types.pb.h"

#include "types/locationid.h"

// gui internal states, persistent between the starts of the program
class PersistentState
{
public:
    static PersistentState &instance()
    {
        static PersistentState s;
        return s;
    }

    void setFirewallState(bool bFirewallOn);
    bool isFirewallOn() const;

    bool isWindowPosExists() const;
    void setWindowPos(const QPoint &windowOffs);
    QPoint windowPos() const;

    void setCountVisibleLocations(int cnt);
    int countVisibleLocations() const;

    void setFirstRun(bool bFirstRun);
    bool isFirstRun();

    void setIgnoreCpuUsageWarnings(bool isIgnore);
    bool isIgnoreCpuUsageWarnings();

    void setLastLocation(const LocationID &lid);
    LocationID lastLocation() const;

    void setLastExternalIp(const QString &ip);
    QString lastExternalIp() const;

    void setNetworkWhitelist(const ProtoTypes::NetworkWhiteList &list);
    ProtoTypes::NetworkWhiteList networkWhitelist() const;





private:
    PersistentState();
    void loadFromVersion1();
    void load();
    void save();

    ProtoTypes::GuiPersistentState state_;
};

#endif // PERSISTENTSTATE_H
