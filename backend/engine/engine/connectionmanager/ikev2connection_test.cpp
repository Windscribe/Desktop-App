#include "ikev2connection_test.h"

#include <QPauseAnimation>
#include <QState>

// signals
// void connected();
// void disconnected();
// void reconnecting();
// void error(CONNECTION_ERROR err);
// void statisticsUpdated(quint64 bytesIn, quint64 bytesOut);

// errors for win
// AUTH_ERROR
// IKEV_FAILED_TO_CONNECT
// IKEV_NOT_FOUND_WIN
// IKEV_FAILED_SET_ENTRY_WIN

// errors for mac
// AUTH_ERROR
// IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC
// IKEV_FAILED_SET_KEYCHAIN_MAC
// IKEV_FAILED_LOAD_PREFERENCES_MAC
// IKEV_FAILED_SAVE_PREFERENCES_MAC
// IKEV_FAILED_START_MAC
// IKEV_FAILED_TO_CONNECT


IKEv2Connection_test::IKEv2Connection_test(QObject *parent, IHelper *helper)
    : IConnection(parent, helper), isConnected_(false), stateMachine_(nullptr)
{
}

IKEv2Connection_test::~IKEv2Connection_test()
{
}

void IKEv2Connection_test::startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName, const QString &username, const QString &password, const ProxySettings &proxySettings, const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode)
{
    Q_UNUSED(configPathOrUrl);
    Q_UNUSED(ip);
    Q_UNUSED(dnsHostName);
    Q_UNUSED(username);
    Q_UNUSED(password);
    Q_UNUSED(proxySettings);
    Q_UNUSED(wireGuardConfig);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isAutomaticConnectionMode);

    if (stateMachine_)
    {
        delete stateMachine_;
        stateMachine_ = NULL;
    }

    isConnected_ = false;

    // test case 1
    // click connect wait 1000 sec, connected
    // click disconnect, disconnected
    QState *state1 = new QState();
    connect(state1, &QState::entered, this, [this]
    {
        QThread::msleep(2000);
        emit error(IKEV_FAILED_TO_CONNECT);
        //emit error(AUTH_ERROR);
        //isConnected_ = false;
        //emit disconnected();
    });
    stateMachine_ = new QStateMachine(this);
    stateMachine_->addState(state1);
    stateMachine_->setInitialState(state1);
    stateMachine_->start();
}

void IKEv2Connection_test::startDisconnect()
{
    if (isConnected_)
    {
        if (stateMachine_)
        {
            delete stateMachine_;
            stateMachine_ = NULL;
        }
        isConnected_ = false;
        emit disconnected();
    }
    else
    {
        emit disconnected();
    }
}

bool IKEv2Connection_test::isDisconnected() const
{
    return !isConnected_;
}

void IKEv2Connection_test::removeIkev2ConnectionFromOS()
{
}

