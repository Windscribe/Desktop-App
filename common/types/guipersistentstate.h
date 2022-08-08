#ifndef TYPES_GUIPERSISTENTSTATE_H
#define TYPES_GUIPERSISTENTSTATE_H

#include <QJsonArray>
#include <QString>
#include "locationid.h"
#include "networkinterface.h"

namespace types {

struct GuiPersistentState
{
    GuiPersistentState() :
        isFirewallOn(false),
        windowOffsX(INT_MAX),
        windowOffsY(INT_MAX),
        countVisibleLocations(7),
        isFirstLogin(true),
        isIgnoreCpuUsageWarnings(false),
        lastExternalIp("N/A")
    {}

    bool isFirewallOn;
    qint32 windowOffsX;
    qint32 windowOffsY;
    qint32 countVisibleLocations;
    bool isFirstLogin;
    bool isIgnoreCpuUsageWarnings;
    LocationID lastLocation;
    QString lastExternalIp;
    QVector<types::NetworkInterface> networkWhiteList;

    bool isWindowOffsSetled() const
    {
        return windowOffsX != INT_MAX && windowOffsY != INT_MAX;
    }

    bool operator==(const GuiPersistentState &other) const
    {
        return other.isFirewallOn == isFirewallOn &&
               other.windowOffsX == windowOffsX &&
               other.windowOffsY == windowOffsY &&
               other.countVisibleLocations == countVisibleLocations &&
               other.isFirstLogin == isFirstLogin &&
               other.isIgnoreCpuUsageWarnings == isIgnoreCpuUsageWarnings &&
               other.lastLocation == lastLocation &&
               other.lastExternalIp == lastExternalIp &&
               other.networkWhiteList == networkWhiteList;
    }

    bool operator!=(const GuiPersistentState &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJsonObject() const
    {
        QJsonObject json;
        json["version"] = versionForSerialization_;
        json["isFirewallOn"] = isFirewallOn;
        json["windowOffsX"] = windowOffsX;
        json["windowOffsY"] = windowOffsY;
        json["countVisibleLocations"] = countVisibleLocations;
        json["isFirstLogin"] = isFirstLogin;
        json["isIgnoreCpuUsageWarnings"] = isIgnoreCpuUsageWarnings;
        json["lastLocation"] = lastLocation.toJsonObject();
        json["lastExternalIp"] = lastExternalIp;

        QJsonArray arr;
        for (const auto &it : networkWhiteList)
        {
            arr << it.toJsonObject();
        }
        json["networkWhiteList"] = arr;
        return json;
    }
    bool fromJsonObject(const QJsonObject &json)
    {
        if (!json.contains("version") || json["version"].toInt(INT_MAX) > versionForSerialization_)
        {
            return false;
        }

        if (json.contains("isFirewallOn")) isFirewallOn = json["isFirewallOn"].toBool(false);
        if (json.contains("windowOffsX")) windowOffsX = json["windowOffsX"].toInt(INT_MAX);
        if (json.contains("windowOffsY")) windowOffsY = json["windowOffsY"].toInt(INT_MAX);
        if (json.contains("countVisibleLocations")) countVisibleLocations = json["countVisibleLocations"].toInt(7);
        if (json.contains("isFirstLogin")) isFirstLogin = json["isFirstLogin"].toBool(true);
        if (json.contains("isIgnoreCpuUsageWarnings")) isIgnoreCpuUsageWarnings = json["isIgnoreCpuUsageWarnings"].toBool(false);
        if (json.contains("lastLocation")) lastLocation.fromJsonObject(json["lastLocation"].toObject());
        if (json.contains("lastExternalIp")) lastExternalIp = json["lastExternalIp"].toString();
        if (json.contains("networkWhiteList") && json["networkWhiteList"].isArray())
        {
            QJsonArray arr = json["networkWhiteList"].toArray();
            for (const auto &it : arr)
            {
                types::NetworkInterface ni;
                if (ni.fromJsonObject(it.toObject()))
                {
                    networkWhiteList << ni;
                }
            }
        }
        return true;
    }

private:
    static constexpr int versionForSerialization_ = 1;  // should increment the version if the data format is changed
};


} // types namespace

#endif // TYPES_GUIPERSISTENTSTATE_H
