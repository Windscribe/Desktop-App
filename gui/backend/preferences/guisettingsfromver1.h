#ifndef GUISETTINGSFROMVER1_H
#define GUISETTINGSFROMVER1_H

#include "ipc/generated_proto/types.pb.h"

// for read GUI settings from version 1, can be removed in the future
class GuiSettingsFromVer1
{
public:
    static ProtoTypes::GuiSettings read();
};

#endif // GUISETTINGSFROMVER1_H
