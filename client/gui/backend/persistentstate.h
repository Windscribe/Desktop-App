#ifndef PERSISTENTSTATE_H
#define PERSISTENTSTATE_H

#include "types/locationid.h"
#include "types/networkinterface.h"
#include "types/guipersistentstate.h"

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

    void setFirstLogin(bool bFirstRun);
    bool isFirstLogin();

    void setIgnoreCpuUsageWarnings(bool isIgnore);
    bool isIgnoreCpuUsageWarnings();

    void setLastLocation(const LocationID &lid);
    LocationID lastLocation() const;

    void setLastExternalIp(const QString &ip);
    QString lastExternalIp() const;

    void setNetworkWhitelist(const QVector<types::NetworkInterface> &list);
    QVector<types::NetworkInterface> networkWhitelist() const;


private:
    PersistentState();
    void load();
    void save();

    types::GuiPersistentState state_;
};

#endif // PERSISTENTSTATE_H
