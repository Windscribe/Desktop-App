#include "persistentstate.h"

#include <QRect>
#include <QSettings>
#include "utils/logger.h"

void PersistentState::load()
{
    /*QSettings settings;

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

    qCDebugMultiline(LOG_BASIC) << "Gui internal settings:" << QString::fromStdString(censoredState.DebugString());*/
}

void PersistentState::save()
{
    /*QSettings settings;

    int size = (int)state_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    state_.SerializeToArray(arr.data(), size);

    settings.setValue("persistentGuiSettings", arr);*/
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

