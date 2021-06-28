#include "openvpnconnection_linux.h"


OpenVPNConnection_linux::OpenVPNConnection_linux(QObject *parent, IHelper *helper) : IConnection(parent)
{
    isConnected_ = false;
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimer()));
}

OpenVPNConnection_linux::~OpenVPNConnection_linux()
{

}

void OpenVPNConnection_linux::startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName, const QString &username, const QString &password,
                                     const ProxySettings &proxySettings, const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode)
{
    timer_.start(5000);
}

void OpenVPNConnection_linux::startDisconnect()
{
    timer_.stop();
    isConnected_ = false;
    emit disconnected();
}

bool OpenVPNConnection_linux::isDisconnected() const
{
    return !isConnected_;
}

bool OpenVPNConnection_linux::isAllowFirewallAfterCustomConfigConnection() const
{
    return true;
}

void OpenVPNConnection_linux::continueWithUsernameAndPassword(const QString &username, const QString &password)
{
}

void OpenVPNConnection_linux::continueWithPassword(const QString &password)
{
}



void OpenVPNConnection_linux::run()
{

}

void OpenVPNConnection_linux::onTimer()
{
    timer_.stop();
    isConnected_ = true;
    emit connected(AdapterGatewayInfo());
}


