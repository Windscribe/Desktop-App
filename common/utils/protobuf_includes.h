#ifndef PROTOBUF_INCLUDES_H
#define PROTOBUF_INCLUDES_H

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable: 4018 4100 4267)
#endif

#include "ipc/generated_proto/apiinfo.pb.h"
#include "ipc/generated_proto/types.pb.h"
#include "ipc/generated_proto/servercommands.pb.h"
#include "ipc/generated_proto/clientcommands.pb.h"
#include <google/protobuf/util/message_differencer.h>

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#endif // PROTOBUF_INCLUDES_H
