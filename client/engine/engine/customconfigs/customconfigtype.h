#ifndef CUSTOMCONFIGTYPE_H
#define CUSTOMCONFIGTYPE_H

#include "utils/protobuf_includes.h"

namespace customconfigs {

enum CUSTOM_CONFIG_TYPE { CUSTOM_CONFIG_OPENVPN, CUSTOM_CONFIG_WIREGUARD };


ProtoTypes::CustomConfigType customConfigTypeToProtobuf(CUSTOM_CONFIG_TYPE type);

} //namespace customconfigs

#endif // CUSTOMCONFIGTYPE_H
