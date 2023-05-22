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

    bool haveAppGeometry() const;
    void setAppGeometry(const QByteArray& geometry);
    const QByteArray& appGeometry() const;

    bool havePreferencesWindowHeight() const;
    void setPreferencesWindowHeight(int height);
    int preferencesWindowHeight() const;

    LOCATION_TAB lastLocationTab() const;
    void setLastLocationTab(LOCATION_TAB tab);

    void save();

private:
    PersistentState();
    void load();

    types::GuiPersistentState state_;

    // for serialization
    static constexpr quint32 magic_ = 0x8845C2AE;
    static constexpr int versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

#endif // PERSISTENTSTATE_H
