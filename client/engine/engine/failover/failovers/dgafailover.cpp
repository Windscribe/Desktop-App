#include "dgafailover.h"

#include "utils/logger.h"
#include "utils/dga_library.h"
#include "utils/dga_parameters.h"


namespace failover {

void DgaFailover::getData(bool)
{
    DgaLibrary dga;
    if (dga.load()) {
        QString domain = dga.getParameter(PAR_RANDOM_GENERATED_DOMAIN);
        if (domain.isEmpty()) {
            qCDebug(LOG_BASIC) << "dga domain is empty, skip it";
            emit finished(QVector<FailoverData>());
        } else {
            emit finished(QVector<FailoverData>() << FailoverData(domain));
        }

    } else {
        qCDebug(LOG_BASIC) << "No dga, skip it";
        emit finished(QVector<FailoverData>());
    }
}

} // namespace failover
