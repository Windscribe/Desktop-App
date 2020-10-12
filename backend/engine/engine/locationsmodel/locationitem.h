#ifndef LOCATIONSMODEL_LOCATIONITEM_H
#define LOCATIONSMODEL_LOCATIONITEM_H

#include <QString>
#include <QVector>
#include "pingtime.h"
#include "types/locationid.h"
#include "engine/customconfigs/customconfigtype.h"

namespace locationsmodel {

struct CityItem
{
    LocationID id;
    QString city;
    QString nick;
    PingTime pingTimeMs;
    bool isPro;
    bool isDisabled;

    // specific for static IP location
    QString staticIpCountryCode;
    QString staticIpType;
    QString staticIp;

    // specific for custom config location
    customconfigs::CUSTOM_CONFIG_TYPE customConfigType;
    bool customConfigIsCorrect;
    QString customConfigErrorMessage;

    // set default values
    CityItem() : isPro(false), isDisabled(false), customConfigIsCorrect(false), customConfigType(customconfigs::CUSTOM_CONFIG_OPENVPN) {}
};

struct LocationItem
{
    LocationID id;
    QString name;
    QString countryCode;
    bool isPremiumOnly;
    int p2p;
    QString staticIpsDeviceName;            // used only for static Ips location
    QVector<CityItem> cities;

    // set default values
    LocationItem() : isPremiumOnly(false), p2p(0) {}

    void fillProtobuf(ProtoTypes::Location *l) const
    {
        *l->mutable_id() = id.toProtobuf();
        l->set_name(name.toStdString());
        l->set_country_code(countryCode.toStdString());
        l->set_is_premium_only(isPremiumOnly);
        l->set_is_p2p_supported(p2p == 0);
        l->set_static_ip_device_name(staticIpsDeviceName.toStdString());

        for (const locationsmodel::CityItem &ci : cities)
        {
            ProtoTypes::City *city = l->add_cities();
            *city->mutable_id() = ci.id.toProtobuf();
            city->set_name(ci.city.toStdString());
            city->set_nick(ci.nick.toStdString());
            city->set_ping_time(ci.pingTimeMs.toInt());
            city->set_is_premium_only(ci.isPro);
            city->set_is_disabled(ci.isDisabled);

            city->set_static_ip_country_code(ci.staticIpCountryCode.toStdString());
            city->set_static_ip_type(ci.staticIpType.toStdString());
            city->set_static_ip(ci.staticIp.toStdString());

            city->set_custom_config_type(customconfigs::customConfigTypeToProtobuf(ci.customConfigType));
            city->set_custom_config_is_correct(ci.customConfigIsCorrect);
            city->set_custom_config_error_message(ci.customConfigErrorMessage.toStdString());
        }
    }
};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_LOCATIONITEM_H
