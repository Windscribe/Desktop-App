#include "ikev2connection_linux.h"
#include "utils/logger.h"
#include <QCoreApplication>



IKEv2Connection_linux::IKEv2Connection_linux(QObject *parent, IHelper *helper) : IConnection(parent)
{
}

IKEv2Connection_linux::~IKEv2Connection_linux()
{
}

void IKEv2Connection_linux::startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName, const QString &username, const QString &password, const ProxySettings &proxySettings, const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode)
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

void IKEv2Connection_linux::continueWithUsernameAndPassword(const QString &/*username*/, const QString &/*password*/)
{
    // nothing todo for ikev2
    Q_ASSERT(false);
}


void IKEv2Connection_linux::continueWithPassword(const QString & /*password*/)
{
    // nothing todo for ikev2
    Q_ASSERT(false);
}

void IKEv2Connection_linux::fakeImpl()
{
    emit error(ProtoTypes::ConnectError::IKEV_NOT_FOUND_WIN);
}

