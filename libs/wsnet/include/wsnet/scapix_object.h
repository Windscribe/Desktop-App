#pragma once

// Scapix optional integration ( more info -> https://www.scapix.com/language_bridge/optional_integration )

#ifdef SCAPIX_BRIDGE

#include <scapix/bridge/object.h>

template <typename T>
using scapix_object = scapix::bridge::object<T>;

#else

template <typename T>
class scapix_object {};

#endif
