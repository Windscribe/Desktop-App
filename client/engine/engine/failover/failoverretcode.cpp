#include "failoverretcode.h"
#include "utils/ws_assert.h"

namespace failover {

QDebug operator<<(QDebug dbg, const FailoverRetCode &f)
{
    QDebugStateSaver saver(dbg);
    switch (f) {
    case FailoverRetCode::kSuccess:
        dbg << "kSuccess";
        break;
    case FailoverRetCode::kSslError:
        dbg << "kSslError";
        break;
    case FailoverRetCode::kConnectStateChanged:
        dbg << "kConnectStateChanged";
        break;
    case FailoverRetCode::kFailed:
        dbg << "kFailed";
        break;
    default:
        WS_ASSERT(false);
    }

    return dbg;
}

} // namespace failover

