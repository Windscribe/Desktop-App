#include "persistentstate.h"

#include <QRect>
#include <QSettings>
#include "utils/logger.h"

void PersistentState::load()
{
    QSettings settings;

    if (settings.contains("persistentGuiSettings"))
    {
        QByteArray arr = settings.value("persistentGuiSettings").toByteArray();
        state_.ParseFromArray(arr.data(), arr.size());
    }
    // if can't load from version 2
    // then try load from version 1
    else
    {
        loadFromVersion1();
    }

    // Censor User Ip from log
    ProtoTypes::GuiPersistentState censoredState = state_;
    QString censoredIp = QString::fromStdString(censoredState.last_external_ip());
    censoredIp = censoredIp.mid(0, censoredIp.lastIndexOf('.')+1) + "###"; // censor last octet
    censoredState.set_last_external_ip(censoredIp.toStdString());

    qCDebug(LOG_BASIC) << "Gui internal settings:" << QString::fromStdString(censoredState.DebugString());
}

void PersistentState::save()
{
    QSettings settings;

    int size = state_.ByteSize();
    QByteArray arr(size, Qt::Uninitialized);
    state_.SerializeToArray(arr.data(), size);

    settings.setValue("persistentGuiSettings", arr);
}

void PersistentState::setFirewallState(bool bFirewallOn)
{
    state_.set_is_firewall_on(bFirewallOn);
    save();
}

bool PersistentState::isFirewallOn() const
{
    return state_.is_firewall_on();
}

bool PersistentState::isWindowPosExists() const
{
    return state_.has_window_offs_x() && state_.has_window_offs_y();
}

void PersistentState::setWindowPos(const QPoint &windowOffs)
{
    state_.set_window_offs_x(windowOffs.x());
    state_.set_window_offs_y(windowOffs.y());
    save();
}

QPoint PersistentState::windowPos() const
{
    QPoint pt(state_.window_offs_x(), state_.window_offs_y());
    return pt;
}

void PersistentState::setCountVisibleLocations(int cnt)
{
    state_.set_count_visible_locations(cnt);
    save();
}

int PersistentState::countVisibleLocations() const
{
    return state_.count_visible_locations();
}

void PersistentState::setFirstRun(bool bFirstRun)
{
    state_.set_is_first_run(bFirstRun);
    save();
}

bool PersistentState::isFirstRun()
{
    return state_.is_first_run();
}

void PersistentState::setIgnoreCpuUsageWarnings(bool isIgnore)
{
    state_.set_is_ignore_cpu_usage_warnings(isIgnore);
    save();
}

bool PersistentState::isIgnoreCpuUsageWarnings()
{
    return state_.is_ignore_cpu_usage_warnings();
}

void PersistentState::setLastLocation(const LocationID &lid)
{
    *state_.mutable_lastlocation() = lid.toProtobuf();
    save();
}

LocationID PersistentState::lastLocation() const
{
    return LocationID::createFromProtoBuf(state_.lastlocation());
}

void PersistentState::setLastExternalIp(const QString &ip)
{
    state_.set_last_external_ip(ip.toStdString());
    save();
}

QString PersistentState::lastExternalIp() const
{
    return QString::fromStdString(state_.last_external_ip());
}

void PersistentState::setNetworkWhitelist(const ProtoTypes::NetworkWhiteList &list)
{
    *state_.mutable_network_white_list() = list;
    save();
}

ProtoTypes::NetworkWhiteList PersistentState::networkWhitelist() const
{
    return state_.network_white_list();
}

PersistentState::PersistentState()
{
    load();
}

void PersistentState::loadFromVersion1()
{
    QSettings settings("Windscribe", "Windscribe");
    if (settings.contains("firewallChecked"))
    {
        state_.set_is_firewall_on(settings.value("firewallChecked").toBool());
    }

    if (settings.contains("windowGeometry"))
    {
        QRect windowGeometry = settings.value("windowGeometry").toRect();
        state_.set_window_offs_x(windowGeometry.x());
        state_.set_window_offs_y(windowGeometry.y());
    }

    if (settings.contains("firstRun"))
    {
        state_.set_is_first_run(settings.value("firstRun").toBool());
    }

    if (settings.contains("ignoreCpuUsageWarnings"))
    {
        state_.set_is_ignore_cpu_usage_warnings(settings.value("ignoreCpuUsageWarnings").toBool());
    }

    if (settings.contains("lastExternalIp"))
    {
        state_.set_last_external_ip(settings.value("lastExternalIp").toString().toStdString());
    }

    if (settings.contains("countVisibleLocations"))
    {
        state_.set_count_visible_locations(settings.value("countVisibleLocations", 7).toInt());
    }
}
