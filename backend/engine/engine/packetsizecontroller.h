#ifndef PACKETSIZECONTROLLER_H
#define PACKETSIZECONTROLLER_H

#include <QObject>
#include <QMutex>
#include "IPC/generated_proto/types.pb.h"
#include "NetworkStateManager/inetworkstatemanager.h"

class PacketSizeController : public QObject
{
    Q_OBJECT
public:
    explicit PacketSizeController(QObject *parent = nullptr);

    void setPacketSize(const ProtoTypes::PacketSize &packetSize);
    void detectPacketSizeMss();
    void earlyStop();
signals:
    void packetSizeChanged(bool isAuto, int mtu);
    void finishedPacketSizeDetection();

private slots:
    void detectPacketSizeMssImpl();

private:
    QMutex mutex_;
    bool earlyStop_;
    ProtoTypes::PacketSize packetSize_;
    void setPacketSizeImpl(const ProtoTypes::PacketSize &packetSize);

    int getIdealPacketSize();
};

#endif // PACKETSIZECONTROLLER_H
