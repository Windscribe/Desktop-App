#include "ikev2connection_linux.h"
#include <QCoreApplication>

IKEv2Connection_linux::IKEv2Connection_linux(QObject *parent, Helper *helper, types::Protocol protocol, const Ikev2SessionParams &sessionParams) :
    Ikev2ConnectionBase(parent, protocol, sessionParams)
{
    Q_UNUSED(helper);
}

IKEv2Connection_linux::~IKEv2Connection_linux()
{
}

void IKEv2Connection_linux::startConnect()
{
    QMetaObject::invokeMethod(this, "fakeImpl");
}

void IKEv2Connection_linux::startDisconnect()
{
    emit disconnected();
}

bool IKEv2Connection_linux::isDisconnected() const
{
    return  true;
}

void IKEv2Connection_linux::fakeImpl()
{
    emit error(IKEV_NOT_FOUND_WIN);
}

