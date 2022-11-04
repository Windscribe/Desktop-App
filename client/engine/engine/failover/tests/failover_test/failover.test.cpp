#include <QtTest>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
//#include <WinSock2.h>
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/failover/failover.h"

class ConnectStateController_moc : public IConnectStateController
{
    Q_OBJECT
public:
    explicit ConnectStateController_moc(QObject *parent) : IConnectStateController(parent) {}

    CONNECT_STATE currentState() override
    {
        return state_;
    }
    CONNECT_STATE prevState() override
    {
        return CONNECT_STATE_DISCONNECTED;
    }

    DISCONNECT_REASON disconnectReason() override
    {
        return DISCONNECTED_ITSELF;
    }
    CONNECT_ERROR connectionError() override
    {
        return NO_CONNECT_ERROR;
    }
    const LocationID& locationId() override
    {
        return lid_;
    }

    void changeState()
    {
        prevState_ = state_;
        if (state_ == CONNECT_STATE_DISCONNECTED)
            state_ = CONNECT_STATE_CONNECTED;
        else
            state_ = CONNECT_STATE_DISCONNECTED;

        emit stateChanged(state_, DISCONNECTED_ITSELF, NO_CONNECT_ERROR, lid_);
    }

private:
    LocationID lid_;
    CONNECT_STATE state_ = CONNECT_STATE_DISCONNECTED;
    CONNECT_STATE prevState_ = CONNECT_STATE_DISCONNECTED;

};


// This is a manual test, todo some kind of automatic test is not trivial
// Just run and see the order of domain traversal in the log
class TestFailover : public QObject
{
    Q_OBJECT

public:
    TestFailover();
    ~TestFailover();

private slots:
    void basicTest();
    void manualDnsResolutionTest();
};


TestFailover::TestFailover()
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
}

TestFailover::~TestFailover()
{
}

void TestFailover::basicTest()
{
    QScopedPointer<ConnectStateController_moc> connectStateController(new ConnectStateController_moc(this));
    QScopedPointer<NetworkAccessManager> accessManager(new  NetworkAccessManager(this));
    QScopedPointer<failover::Failover> failover(new failover::Failover(this, accessManager.get(), connectStateController.get()));

    connect(failover.get(), &failover::Failover::nextHostnameAnswer, [=](failover::FailoverRetCode retCode, const QString &hostname) {
        if (retCode == failover::FailoverRetCode::kSuccess)
            qDebug() << "next hostname:" <<  hostname;
    });

    connect(failover.get(), &failover::Failover::tryingBackupEndpoint, [=](int num, int cnt) {
        qDebug() << QString("Trying Backup Endpoints %1/%2").arg(num).arg(cnt);
    });

    while (true) {
        failover->getNextHostname(false);
        //connectStateController->changeState();
        QSignalSpy spy(failover.get(), SIGNAL(nextHostnameAnswer(failover::FailoverRetCode, QString)));
        spy.wait(60000);
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        if (arguments.at(0).value<failover::FailoverRetCode>() == failover::FailoverRetCode::kFailed)
            break;
    };
}

void TestFailover::manualDnsResolutionTest()
{
    QScopedPointer<ConnectStateController_moc> connectStateController(new ConnectStateController_moc(this));
    QScopedPointer<NetworkAccessManager> accessManager(new  NetworkAccessManager(this));
    QScopedPointer<failover::Failover> failover(new failover::Failover(this, accessManager.get(), connectStateController.get()));
    types::ApiResolutionSettings apiResolutionSettings;
    apiResolutionSettings.set(false, "1.1.1.1");
    failover->setApiResolutionSettings(apiResolutionSettings);

    connect(failover.get(), &failover::Failover::nextHostnameAnswer, [=](failover::FailoverRetCode retCode, const QString &hostname) {
        if (retCode == failover::FailoverRetCode::kSuccess)
            qDebug() << "next hostname:" <<  hostname;
    });

    connect(failover.get(), &failover::Failover::tryingBackupEndpoint, [=](int num, int cnt) {
        qDebug() << QString("Trying Backup Endpoints %1/%2").arg(num).arg(cnt);
    });

    while (true) {
        failover->getNextHostname(false);
        QSignalSpy spy(failover.get(), SIGNAL(nextHostnameAnswer(failover::FailoverRetCode, QString)));
        spy.wait(60000);
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        if (arguments.at(0).value<failover::FailoverRetCode>() == failover::FailoverRetCode::kFailed)
            break;
    };
}

QTEST_MAIN(TestFailover)
#include "failover.test.moc"
