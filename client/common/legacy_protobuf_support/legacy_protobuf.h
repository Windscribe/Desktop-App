#ifndef LEGACY_PROTOBUF_H
#define LEGACY_PROTOBUF_H

#include "types/guisettings.h"
#include "types/enginesettings.h"
#include "types/guipersistentstate.h"

// try load from legacy protobuf
// todo remove this code at some point later
class LegacyProtobufSupport
{
public:
    static bool loadGuiSettings(const QByteArray &arr, types::GuiSettings &out);
    static bool loadEngineSettings(const QByteArray &arr, types::EngineSettings &out);
    static bool loadGuiPersistentState(const QByteArray &arr, types::GuiPersistentState &out);

    static bool loadFavoriteLocations(const QByteArray &arr, QSet<LocationID> &out);
};



#endif // LEGACY_PROTOBUF_H
