#include "dgafailover.h"

#include "utils/logger.h"
#include "utils/dga_library.h"
#include "utils/dga_parameters.h"


namespace failover {

DgaFailover::DgaFailover(QObject *parent, const QString &uniqueId) : BaseFailover(parent, uniqueId)
{
    // postponed for 2.7
    /*DgaLibrary dga;
    if (dga.load()) {
        QString domain = dga.getParameter(PAR_RANDOM_GENERATED_DOMAIN);
        if (domain.isEmpty()) {
            qCDebug(LOG_BASIC) << "dga domain is empty, skip it";
        } else {
            domain_ = domain;
        }

    } else {
        qCDebug(LOG_BASIC) << "No dga, skip it";
    }*/
}

void DgaFailover::getData(bool)
{
    if (!domain_.isEmpty())
        emit finished(QVector<FailoverData>() << FailoverData(domain_));
    else
        emit finished(QVector<FailoverData>());
}

} // namespace failover
