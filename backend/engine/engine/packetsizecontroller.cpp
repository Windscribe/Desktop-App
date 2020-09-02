#include "packetsizecontroller.h"

#include <google/protobuf/util/message_differencer.h>

#include "utils/utils.h"
#include "utils/logger.h"

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

void PacketSizeController::detectPacketSizeMss()
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "detectPacketSizeMssImpl");
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
        emit packetSizeChanged(packetSize.is_automatic(), packetSize.mss());
    }
}

void PacketSizeController::detectPacketSizeMssImpl()
{
    {
        QMutexLocker locker(&mutex_);
        earlyStop_ = false;
    }

    int mss = getIdealPacketSize();

    QMutexLocker locker(&mutex_);
    if (mss != -1)
    {
        qCDebug(LOG_BASIC) << "Found mss: " << mss;
        ProtoTypes::PacketSize packetSize;
        packetSize.set_is_automatic(packetSize_.is_automatic());
        packetSize.set_mss(mss);
        setPacketSizeImpl(packetSize);
    }

    emit finishedPacketSizeDetection();
}

int PacketSizeController::getIdealPacketSize()
{
    int mtu = 1500;

    bool success = false;
    while (mtu >= 1300)
    {
        QMutexLocker locker(&mutex_);
        if (earlyStop_)
        {
            qCDebug(LOG_BASIC) << "Exiting packet size detection loop early";
            break;
        }

        bool pingSuccess = Utils::pingWithMtu(mtu);
        if (pingSuccess)
        {
            success = true;
            break;
        }
        mtu -= 10;
    }

    if (!success)
    {
        qCDebug(LOG_BASIC) << "Couldn't find appropriate MTU -- check internet connection";
        return -1;
    }

    return mtu - 40;
}
