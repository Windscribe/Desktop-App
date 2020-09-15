#include "protoenumtostring.h"


QString ProtoEnumToString::toString(const QString &protoEnumName)
{
    auto it = map_.find(protoEnumName);
    if (it != map_.end())
    {
        return it.value();
    }
    else
    {
        Q_ASSERT(false);
        return "Unknown string";
    }
}

ProtoEnumToString::ProtoEnumToString()
{
    map_["PROTOCOL_IKEV2"] = "IKEv2";
    map_["PROTOCOL_UDP"] = "UDP";
    map_["PROTOCOL_TCP"] = "TCP";
    map_["PROTOCOL_STUNNEL"] = "Stealth";
    map_["PROTOCOL_WSTUNNEL"] = "WStunnel";
    map_["PROTOCOL_WIREGUARD"] = "WireGuard";

    map_["ORDER_LOCATION_BY_GEOGRAPHY"] = "Geography";
    map_["ORDER_LOCATION_BY_ALPHABETICALLY"] = "Alphabet";
    map_["ORDER_LOCATION_BY_LATENCY"] = "Latency";

    map_["LATENCY_DISPLAY_BARS"] = "Bars";
    map_["LATENCY_DISPLAY_MS"] = "Ms";

    map_["PROXY_SHARING_HTTP"] = "HTTP";
    map_["PROXY_SHARING_SOCKS"] = "SOCKS";

    map_["UPDATE_CHANNEL_RELEASE"] = "Release";
    map_["UPDATE_CHANNEL_BETA"] = "Beta";
    map_["UPDATE_CHANNEL_GUINEA_PIG"] = "Guinea Pig";

    map_["FIREWALL_MODE_MANUAL"] = "Manual";
    map_["FIREWALL_MODE_AUTOMATIC"] = "Automatic";
    map_["FIREWALL_MODE_ALWAYS_ON"] = "Always On";

    map_["FIREWALL_WHEN_BEFORE_CONNECTION"] = "Before Connection";
    map_["FIREWALL_WHEN_AFTER_CONNECTION"] = "After Connection";

    map_["PROXY_OPTION_NONE"] = "None";
    map_["PROXY_OPTION_AUTODETECT"] = "Auto-detect";
    map_["PROXY_OPTION_HTTP"] = "HTTP";
    map_["PROXY_OPTION_SOCKS"] = "SOCKS";

    map_["DNS_POLICY_OS_DEFAULT"] = "OS Default";
    map_["DNS_POLICY_OPEN_DNS"] = "Open DNS";
    map_["DNS_POLICY_CLOUDFLARE"] = "Cloudflare";
    map_["DNS_POLICY_GOOGLE"] = "Google";

    map_["TAP_ADAPTER_NOT_INSTALLED"] = "Not installed";
    map_["TAP_ADAPTER_5"] = "Legacy Driver (NDIS5)";
    map_["TAP_ADAPTER"] = "Windscribe VPN";
    map_["WINTUN_ADAPTER"] = "Wintun";

}

QString ProtoEnumToString::toString(ProtoTypes::Protocol p)
{
    const google::protobuf::EnumDescriptor *descriptor = ProtoTypes::Protocol_descriptor();
    std::string name = descriptor->FindValueByNumber(p)->name();
    return toString(QString::fromStdString(name));
}

QString ProtoEnumToString::toString(ProtoTypes::ProxySharingMode p)
{
    const google::protobuf::EnumDescriptor *descriptor = ProtoTypes::ProxySharingMode_descriptor();
    std::string name = descriptor->FindValueByNumber(p)->name();
    return toString(QString::fromStdString(name));
}

QString ProtoEnumToString::toString(ProtoTypes::OrderLocationType p)
{
    const google::protobuf::EnumDescriptor *descriptor = ProtoTypes::OrderLocationType_descriptor();
    std::string name = descriptor->FindValueByNumber(p)->name();
    return toString(QString::fromStdString(name));
}

QString ProtoEnumToString::toString(ProtoTypes::LatencyDisplayType p)
{
    const google::protobuf::EnumDescriptor *descriptor = ProtoTypes::LatencyDisplayType_descriptor();
    std::string name = descriptor->FindValueByNumber(p)->name();
    return toString(QString::fromStdString(name));
}

QString ProtoEnumToString::toString(ProtoTypes::TapAdapterType p)
{
    const google::protobuf::EnumDescriptor *descriptor = ProtoTypes::TapAdapterType_descriptor();
    std::string name = descriptor->FindValueByNumber(p)->name();
    return toString(QString::fromStdString(name));
}


QList<QPair<QString, int> > ProtoEnumToString::getEnums(const google::protobuf::EnumDescriptor *descriptor)
{
    QList<QPair<QString, int> > list;
    for (int i = 0; i < descriptor->value_count(); ++i)
    {
        const google::protobuf::EnumValueDescriptor *v = descriptor->value(i);
        list << QPair<QString, int>( toString(QString::fromStdString(v->name())), v->number() );
    }

    return list;
}
