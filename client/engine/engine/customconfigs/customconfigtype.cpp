#include "customconfigtype.h"
#include <QObject>

namespace customconfigs {

ProtoTypes::CustomConfigType customConfigTypeToProtobuf(CUSTOM_CONFIG_TYPE type)
{
    if (type == CUSTOM_CONFIG_OPENVPN)
    {
        return ProtoTypes::CUSTOM_CONFIG_OPENVPN;
    }
    else if (type == CUSTOM_CONFIG_WIREGUARD)
    {
        return ProtoTypes::CUSTOM_CONFIG_WIREGUARD;
    }
    else
    {
        Q_ASSERT(false);
        return ProtoTypes::CUSTOM_CONFIG_OPENVPN;
    }
}


} //namespace customconfigs
