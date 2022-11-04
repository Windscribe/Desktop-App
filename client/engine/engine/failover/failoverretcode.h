#pragma once
#include <QDebug>

namespace failover {

enum class FailoverRetCode { kSuccess, kSslError, kConnectStateChanged, kFailed };

QDebug operator<<(QDebug dbg, const FailoverRetCode &f);

} // namespace failover
