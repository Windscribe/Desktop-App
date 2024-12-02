#pragma once

#include "scapix_object.h"

namespace wsnet {

// You should call cancel(...) if you don't want the callback to be called. If callback has already been called, then cancel(...) has no effect.
class WSNetCancelableCallback : public scapix_object<WSNetCancelableCallback>
{
public:
    virtual ~WSNetCancelableCallback() {}

    virtual void cancel() = 0;
};

} // namespace wsnet

