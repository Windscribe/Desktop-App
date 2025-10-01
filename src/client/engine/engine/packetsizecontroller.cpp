#include "packetsizecontroller.h"

#include "utils/ipvalidation.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"

PacketSizeController::PacketSizeController(QObject *parent)
    : QObject(parent),
      earlyStop_(false)
{
}

void PacketSizeController::init()
{
#ifdef Q_OS_WIN
    crashHandler_.reset(new Debug::CrashHandlerForThread());
#endif
}

void PacketSizeController::finish()
{
#ifdef Q_OS_WIN
    crashHandler_.reset();
#endif
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
        qCInfo(LOG_PACKET_SIZE) << "Found mtu: " << mtu;
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
    if (IpValidation::isIp(hostname))
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
            qCInfo(LOG_PACKET_SIZE) << "Exiting packet size detection loop early";
            break;
        }

        bool pingSuccess = NetworkUtils::pingWithMtu(modifiedHostname, mtu);
        if (pingSuccess)
        {
            success = true;
            break;
        }
        mtu -= 10;
    }

    if (!success)
    {
        qCWarning(LOG_PACKET_SIZE) << "Couldn't find appropriate MTU -- check internet connection";
        return -1;
    }

    return mtu;
}
