#ifndef PACKETSIZECONTROLLER_H
#define PACKETSIZECONTROLLER_H

#include <QObject>
#include <QMutex>
#include "networkstatemanager/inetworkstatemanager.h"
#include "utils/protobuf_includes.h"

class PacketSizeController : public QObject
{
    Q_OBJECT
public:
    explicit PacketSizeController(QObject *parent = nullptr);

    void setPacketSize(const ProtoTypes::PacketSize &packetSize);
    void detectAppropriatePacketSize(const QString &hostname);
    void earlyStop();
signals:
    void packetSizeChanged(bool isAuto, int mtu);
    void finishedPacketSizeDetection(bool isError);

private slots:
    void detectAppropriatePacketSizeImpl(const QString &hostname);

private:
    QMutex mutex_;
    bool earlyStop_;
    ProtoTypes::PacketSize packetSize_;
    void setPacketSizeImpl(const ProtoTypes::PacketSize &packetSize);

    int getIdealPacketSize(const QString &hostname);
};

#endif // PACKETSIZECONTROLLER_H
