#include "packetsizecontroller.h"

#include <google/protobuf/util/message_differencer.h>
#include "engine/hardcodedsettings.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/ipvalidation.h"

const int typeIdPacketSize = qRegisterMetaType<ProtoTypes::Protocol>("ProtoTypes::PacketSize");

PacketSizeController::PacketSizeController(QObject *parent) : QObject(parent)
  , earlyStop_(false)
{
}

void PacketSizeController::setPacketSize(const ProtoTypes::PacketSize &packetSize)
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

void PacketSizeController::setPacketSizeImpl(const ProtoTypes::PacketSize &packetSize)
{
    if (!google::protobuf::util::MessageDifferencer::Equals(packetSize, packetSize_))
    {
        packetSize_ = packetSize;
        emit packetSizeChanged(packetSize.is_automatic(), packetSize.mtu());
    }
}

void PacketSizeController::detectAppropriatePacketSizeImpl(const QString &hostname)
{
    {
        QMutexLocker locker(&mutex_);
        earlyStop_ = false;
    }

    int mtu = getIdealPacketSize(hostname);

    QMutexLocker locker(&mutex_);
    if (mtu > 0)
    {
        qCDebug(LOG_PACKET_SIZE) << "Found mtu: " << mtu;
        ProtoTypes::PacketSize packetSize;
        packetSize.set_is_automatic(packetSize_.is_automatic());
        packetSize.set_mtu(mtu);
        setPacketSizeImpl(packetSize);
    }

    emit finishedPacketSizeDetection();
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
