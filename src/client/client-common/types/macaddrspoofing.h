#pragma once

#include <QJsonArray>
#include <QString>
#include "networkinterface.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"
#include "utils/networkingvalidation.h"
#ifdef Q_OS_LINUX
#include "utils/network_utils/network_utils_linux.h"
#endif

namespace types {

struct MacAddrSpoofing
{
    MacAddrSpoofing() {}

    MacAddrSpoofing(const QJsonObject &json)
    {
        if (json.contains(kJsonIsEnabledProp) && json[kJsonIsEnabledProp].isBool()) {
            isEnabled = json[kJsonIsEnabledProp].toBool();
        }

        if (json.contains(kJsonMacAddressProp) && json[kJsonMacAddressProp].isString()) {
            macAddress = NetworkUtils::normalizeMacAddress(json[kJsonMacAddressProp].toString());
        }

        if (json.contains(kJsonIsAutoRotateProp) && json[kJsonIsAutoRotateProp].isBool()) {
            isAutoRotate = json[kJsonIsAutoRotateProp].toBool();
        }

        if (json.contains(kJsonSelectedNetworkInterfaceProp) && json[kJsonSelectedNetworkInterfaceProp].isObject()) {
            selectedNetworkInterface = NetworkInterface(json[kJsonSelectedNetworkInterfaceProp].toObject());
        }

        if (json.contains(kJsonNetworkInterfacesProp) && json[kJsonNetworkInterfacesProp].isArray()) {
            QJsonArray networkInterfacesArray = json[kJsonNetworkInterfacesProp].toArray();
            networkInterfaces.clear();
            for (const QJsonValue &ifaceValue : std::as_const(networkInterfacesArray)) {
                if (ifaceValue.isObject()) {
                    NetworkInterface iface(ifaceValue.toObject());
                    networkInterfaces.append(iface);
                }
            }
        }
    }

    bool isEnabled = false;
    QString macAddress;
    bool isAutoRotate = false;
    NetworkInterface selectedNetworkInterface;
    QVector<NetworkInterface> networkInterfaces;

    bool operator==(const MacAddrSpoofing &other) const
    {
        return other.isEnabled == isEnabled &&
               other.macAddress == macAddress &&
               other.isAutoRotate == isAutoRotate &&
               other.selectedNetworkInterface == selectedNetworkInterface &&
               other.networkInterfaces == networkInterfaces;
    }

    bool operator!=(const MacAddrSpoofing &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;

        json[kJsonIsEnabledProp] = isEnabled;
        json[kJsonMacAddressProp] = macAddress;
        json[kJsonIsAutoRotateProp] = isAutoRotate;
        json[kJsonSelectedNetworkInterfaceProp] = selectedNetworkInterface.toJson(isForDebugLog);

        if (!networkInterfaces.isEmpty()) {
            QJsonArray networkInterfacesArray;
            for (const NetworkInterface& iface : networkInterfaces) {
                networkInterfacesArray.append(iface.toJson(isForDebugLog));
            }
            json[kJsonNetworkInterfacesProp] = networkInterfacesArray;
        }

        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const MacAddrSpoofing &o)
    {
        stream << versionForSerialization_;
        stream << o.isEnabled << o.macAddress << o.isAutoRotate << o.selectedNetworkInterface << o.networkInterfaces;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, MacAddrSpoofing &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isEnabled >> o.macAddress >> o.isAutoRotate >> o.selectedNetworkInterface >> o.networkInterfaces;
        return stream;
    }

#ifdef CLI_ONLY
    void fromIni(const QSettings &settings)
    {
        isEnabled = settings.value(kIniIsEnabledProp, false).toBool();
        macAddress = NetworkUtils::normalizeMacAddress(settings.value(kIniMacAddressProp).toString());
        isAutoRotate = settings.value(kIniIsAutoRotateProp, false).toBool();

#ifdef Q_OS_LINUX
        QString name = settings.value(kIniInterfaceProp).toString();
        selectedNetworkInterface = NetworkUtils_linux::networkInterfaceByName(name);
        if (NetworkInterface::isNoNetworkInterface(selectedNetworkInterface.interfaceIndex)) {
            isEnabled = false;
        }
#endif
    }

    void toIni(QSettings &settings) const
    {
        settings.setValue(kIniIsEnabledProp, isEnabled);
        settings.setValue(kIniMacAddressProp, macAddress);
        settings.setValue(kIniIsAutoRotateProp, isAutoRotate);
        settings.setValue(kIniInterfaceProp, selectedNetworkInterface.interfaceName);
    }
#endif

    void validate()
    {
        if (!macAddress.isEmpty() && !NetworkingValidation::isValidMacAddress(macAddress)) {
            qCWarning(LOG_BASIC) << "MacAddrSpoofing: invalid MAC address, disabling spoofing";
            macAddress.clear();
            isEnabled = false;
        }
        if (!selectedNetworkInterface.interfaceName.isEmpty() &&
            !selectedNetworkInterface.isNoNetworkInterface() &&
            !NetworkingValidation::isValidInterfaceName(selectedNetworkInterface.interfaceName)) {
            qCWarning(LOG_BASIC) << "MacAddrSpoofing: invalid interface name, disabling spoofing";
            selectedNetworkInterface = NetworkInterface();
            isEnabled = false;
        }
        selectedNetworkInterface.validate();
        constexpr int kMaxNetworkInterfaces = 64;
        if (networkInterfaces.size() > kMaxNetworkInterfaces) {
            qCWarning(LOG_BASIC) << "MacAddrSpoofing: networkInterfaces over cap, truncating";
            networkInterfaces.resize(kMaxNetworkInterfaces);
        }
        for (auto &nif : networkInterfaces) {
            nif.validate();
        }
    }

private:
    const static inline QString kIniIsEnabledProp = "MACAddressSpoofingEnabled";
    const static inline QString kIniMacAddressProp = "MACAddressSpoofingAddress";
    const static inline QString kIniIsAutoRotateProp = "MACAddressSpoofingAutoRotate";
    const static inline QString kIniInterfaceProp = "MACAddressSpoofingInterface";

    const static inline QString kJsonIsEnabledProp = "isEnabled";
    const static inline QString kJsonMacAddressProp = "macAddress";
    const static inline QString kJsonIsAutoRotateProp = "isAutoRotate";
    const static inline QString kJsonSelectedNetworkInterfaceProp = "selectedNetworkInterface";
    const static inline QString kJsonNetworkInterfacesProp = "networkInterfaces";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} // types namespace
