#include "packetsizecontroller.h"

#include "utils/hardcodedsettings.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/ipvalidation.h"

PacketSizeController::PacketSizeController(QObject *parent) : QObject(parent)
  , earlyStop_(false)
{
}

void PacketSizeController::setPacketSize(const types::PacketSize &packetSize)
{
    QMutexLocker locker(&mutex_);
    setPacketSizeImpl(packetSize);
}

void PacketSizeController::detectAppropriatePacketSize(const QString &hostname)
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "detectAppropriatePacketSizeImpl", Q_ARG(QString, hostname));
}

void PacketSizeController::earlyStop()
{
    QMutexLocker locker(&mutex_);
    earlyStop_ = true;
}

void PacketSizeController::setPacketSizeImpl(const types::PacketSize &packetSize)
{
    if (packetSize != packetSize_)
    {
        packetSize_ = packetSize;
        emit packetSizeChanged(packetSize.isAutomatic, packetSize.mtu);
    }
}

void PacketSizeController::detectAppropriatePacketSizeImpl(const QString &hostname)
{
    {
        QMutexLocker locker(&mutex_);
        earlyStop_ = false;
    }

    const int mtu = getIdealPacketSize(hostname);
    const bool is_error = mtu < 0;

    QMutexLocker locker(&mutex_);
    if (mtu > 0)
    {
        qCDebug(LOG_PACKET_SIZE) << "Found mtu: " << mtu;
        types::PacketSize packetSize;
        packetSize.isAutomatic = packetSize_.isAutomatic;
        packetSize.mtu = mtu;
        setPacketSizeImpl(packetSize);
    }

    emit finishedPacketSizeDetection(is_error);
}

int PacketSizeController::getIdealPacketSize(const QString &hostname)
{
    int mtu = 1470;
    QString modifiedHostname = hostname;

    // if this is IP, use without change
    if (IpValidation::instance().isIp(hostname))
    {
        modifiedHostname = hostname;
    }
    // append "checkip." in the front
    else
    {
        modifiedHostname = "checkip." + hostname;

    }

    qCDebug(LOG_PACKET_SIZE) << "Detecting packet size via:" << modifiedHostname;

    bool success = false;
    while (mtu >= 1300)
    {
        QMutexLocker locker(&mutex_);
        if (earlyStop_)
        {
            qCDebug(LOG_PACKET_SIZE) << "Exiting packet size detection loop early";
            break;
        }

        bool pingSuccess = Utils::pingWithMtu(modifiedHostname, mtu);
        if (pingSuccess)
        {
            success = true;
            break;
        }
        mtu -= 10;
    }

    if (!success)
    {
        qCDebug(LOG_PACKET_SIZE) << "Couldn't find appropriate MTU -- check internet connection";
        return -1;
    }

    return mtu;
}
