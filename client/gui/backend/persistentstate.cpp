#include "persistentstate.h"

#include <QJsonDocument>
#include <QRect>
#include <QSettings>
#include "utils/logger.h"
#include "utils/simplecrypt.h"
#include "types/global_consts.h"

extern "C" {
    #include "legacy_protobuf_support/types.pb-c.h"
}

void PersistentState::load()
{
    QSettings settings;
    bool bLoaded = false;
    if (settings.contains("persistentGuiSettings"))
    {
        SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
        QString str = settings.value("persistentGuiSettings", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QCborValue cbor = QCborValue::fromCbor(arr);
        if (!cbor.isInvalid() && cbor.isMap())
        {
            const QJsonObject &obj = cbor.toJsonValue().toObject();
            if (state_.fromJsonObject(obj))
            {
               bLoaded = true;
            }
        }
        if (!bLoaded)
        {
            // try load from legacy protobuf
            // todo remove this code at some point later
            QByteArray arr = settings.value("persistentGuiSettings").toByteArray();
            ProtoTypes__GuiPersistentState *gs = proto_types__gui_persistent_state__unpack(NULL, arr.size(), (const uint8_t *)arr.data());
            if (gs)
            {
                if (gs->has_is_firewall_on) state_.isFirewallOn = gs->is_firewall_on;
                if (gs->has_window_offs_x) state_.windowOffsX = gs->window_offs_x;
                if (gs->has_window_offs_y) state_.windowOffsY = gs->window_offs_y;
                if (gs->has_count_visible_locations) state_.countVisibleLocations = gs->count_visible_locations;
                if (gs->has_is_first_login) state_.isFirstLogin = gs->is_first_login;
                if (gs->has_is_ignore_cpu_usage_warnings) state_.isIgnoreCpuUsageWarnings = gs->is_ignore_cpu_usage_warnings;
                if (gs->lastlocation && gs->lastlocation->has_id && gs->lastlocation->has_type) {
                    state_.lastLocation = LocationID(gs->lastlocation->id, gs->lastlocation->type, gs->lastlocation->city);
                }
                state_.lastExternalIp = gs->last_external_ip;

                if (gs->network_white_list)
                {
                    state_.networkWhiteList.clear();
                    for (int i = 0; i < gs->network_white_list->n_networks; i++)
                    {
                        types::NetworkInterface networkInterface;

                        ProtoTypes__NetworkInterface *ni = gs->network_white_list->networks[i];

                        if (ni->has_interface_index) {
                            networkInterface.interfaceIndex = ni->interface_index;
                        }
                        networkInterface.interfaceName = ni->interface_name;
                        networkInterface.interfaceGuid = ni->interface_guid;
                        networkInterface.networkOrSSid = ni->network_or_ssid;
                        if (ni->has_interface_type) {
                            networkInterface.interfaceType = (NETWORK_INTERACE_TYPE)ni->interface_type;
                        }
                        if (ni->has_trust_type) {
                            networkInterface.trustType = (NETWORK_TRUST_TYPE)ni->trust_type;
                        }
                        if (ni->has_active) {
                            networkInterface.active = ni->active;
                        }
                        networkInterface.friendlyName = ni->friendly_name;
                        if (ni->has_requested) {
                            networkInterface.requested = ni->requested;
                        }
                        if (ni->has_metric) {
                            networkInterface.metric = ni->metric;
                        }
                        networkInterface.physicalAddress = ni->physical_address;
                        if (ni->has_mtu) {
                            networkInterface.mtu = ni->mtu;
                        }
                        if (ni->has_state) {
                            networkInterface.state = ni->state;
                        }
                        if (ni->has_dw_type) {
                            networkInterface.dwType = ni->dw_type;
                        }
                        networkInterface.deviceName = ni->device_name;
                        if (ni->has_connector_present) {
                            networkInterface.connectorPresent = ni->connector_present;
                        }
                        if (ni->has_end_point_interface) {
                            networkInterface.endPointInterface = ni->end_point_interface;
                        }

                        state_.networkWhiteList << networkInterface;
                    }
                }
                proto_types__gui_persistent_state__free_unpacked(gs, NULL);
            }
        }
    }

    // Censor User Ip from log
    types::GuiPersistentState censoredState = state_;
    QString censoredIp = censoredState.lastExternalIp;
    censoredIp = censoredIp.mid(0, censoredIp.lastIndexOf('.')+1) + "###"; // censor last octet
    censoredState.lastExternalIp = censoredIp;

    QJsonDocument doc(censoredState.toJsonObject());
    QString strJson(doc.toJson(QJsonDocument::Compact));
    qCDebug(LOG_BASIC).noquote() << QString("Gui internal settings: %1").arg(strJson);
}

void PersistentState::save()
{
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    QCborValue cbor = QCborValue::fromJsonValue(state_.toJsonObject());
    QByteArray arr = cbor.toCbor();
    QSettings settings;
    settings.setValue("persistentGuiSettings", simpleCrypt.encryptToString(arr));
}

void PersistentState::setFirewallState(bool bFirewallOn)
{
    state_.isFirewallOn = bFirewallOn;
    save();
}

bool PersistentState::isFirewallOn() const
{
    return state_.isFirewallOn;
}

bool PersistentState::isWindowPosExists() const
{
    return state_.isWindowOffsSetled();
}

void PersistentState::setWindowPos(const QPoint &windowOffs)
{
    state_.windowOffsX = windowOffs.x();
    state_.windowOffsY = windowOffs.y();
    save();
}

QPoint PersistentState::windowPos() const
{
    QPoint pt(state_.windowOffsX, state_.windowOffsY);
    return pt;
}

void PersistentState::setCountVisibleLocations(int cnt)
{
    state_.countVisibleLocations = cnt;
    save();
}

int PersistentState::countVisibleLocations() const
{
    return state_.countVisibleLocations;
}

void PersistentState::setFirstLogin(bool bFirstRun)
{
    state_.isFirstLogin = bFirstRun;
    save();
}

bool PersistentState::isFirstLogin()
{
    return state_.isFirstLogin;
}

void PersistentState::setIgnoreCpuUsageWarnings(bool isIgnore)
{
    state_.isIgnoreCpuUsageWarnings = isIgnore;
    save();
}

bool PersistentState::isIgnoreCpuUsageWarnings()
{
    return state_.isIgnoreCpuUsageWarnings;
}

void PersistentState::setLastLocation(const LocationID &lid)
{
    state_.lastLocation = lid;
    save();
}

LocationID PersistentState::lastLocation() const
{
    return state_.lastLocation;
}

void PersistentState::setLastExternalIp(const QString &ip)
{
    state_.lastExternalIp = ip;
    save();
}

QString PersistentState::lastExternalIp() const
{
    return state_.lastExternalIp;
}

void PersistentState::setNetworkWhitelist(const QVector<types::NetworkInterface> &list)
{
    state_.networkWhiteList = list;
    save();
}

QVector<types::NetworkInterface> PersistentState::networkWhitelist() const
{
    return state_.networkWhiteList;
}

PersistentState::PersistentState()
{
    load();
}

