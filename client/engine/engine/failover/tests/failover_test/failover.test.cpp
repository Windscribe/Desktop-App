#include <QtTest>
#include <QCoreApplication>
#include <QSet>
#include <QtConcurrent/QtConcurrent>
//#include <WinSock2.h>
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/failover/failovercontainer.h"

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
    QScopedPointer<failover::FailoverContainer> failoverContainer(new failover::FailoverContainer(this, accessManager.get()));

    QSet<QString> uniqueIds;
    int failoversCount = 0;

    while (true) {
        QSharedPointer<failover::BaseFailover> failover = failoverContainer->currentFailover();
        uniqueIds << failover->uniqueId();
        QSignalSpy spy(failover.get(), &failover::BaseFailover::finished);
        failover->getData(false);
        while (spy.count() != 1) {
            spy.wait(1000);
        }

        QList<QVariant> arguments = spy.takeFirst();
        qDebug() << failover->name() << arguments.at(0).value<QVector<failover::FailoverData>>();
        failoversCount++;
        if (!failoverContainer->gotoNext())
            break;
    };

    // check that all failover IDs are unique
    QCOMPARE(failoversCount, uniqueIds.count());
}

QTEST_MAIN(TestFailover)
#include "failover.test.moc"
