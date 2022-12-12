#pragma once

#include <QObject>
#include <QMutex>
#include "types/packetsize.h"

#ifdef Q_OS_WIN
    #include "utils/crashhandler.h"
#endif

class PacketSizeController : public QObject
{
    Q_OBJECT
public:
    explicit PacketSizeController(QObject *parent = nullptr);

    void setPacketSize(const types::PacketSize &packetSize);
    void detectAppropriatePacketSize(const QString &hostname);
    void earlyStop();

signals:
    void packetSizeChanged(bool isAuto, int mtu);
    void finishedPacketSizeDetection(bool isError);

public slots:
    void init();
    void finish();

private slots:
    void detectAppropriatePacketSizeImpl(const QString &hostname);

private:
    QMutex mutex_;
    bool earlyStop_;
    types::PacketSize packetSize_;

#ifdef Q_OS_WIN
    QScopedPointer<Debug::CrashHandlerForThread> crashHandler_;
#endif

    void setPacketSizeImpl(const types::PacketSize &packetSize);
    int getIdealPacketSize(const QString &hostname);
};
