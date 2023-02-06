#include "dgafailover.h"

#include "utils/logger.h"
#include "utils/dga_library.h"
#include "utils/dga_parameters.h"


namespace failover {

void DgaFailover::getHostnames(bool)
{
    DgaLibrary dga;
    if (dga.load()) {
        QString domain = dga.getParameter(PAR_RANDOM_GENERATED_DOMAIN);
        if (domain.isEmpty()) {
            qCDebug(LOG_BASIC) << "dga domain is empty, skip it";
            emit finished(FailoverRetCode::kFailed, QStringList());
        } else {
            emit finished(FailoverRetCode::kSuccess, QStringList() << domain);
        }

    } else {
        qCDebug(LOG_BASIC) << "No dga, skip it";
        emit finished(FailoverRetCode::kFailed, QStringList());
    }
}

} // namespace failover
