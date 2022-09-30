#include <QtTest>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <WinSock2.h>
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/serverapi/failover.h"

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
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
}

TestFailover::~TestFailover()
{
}

void TestFailover::basicTest()
{
    NetworkAccessManager *accessManager = new  NetworkAccessManager(this);
    server_api::Failover *failover = new server_api::Failover(this, accessManager);

    connect(failover, &server_api::Failover::nextHostnameAnswer, [=](server_api::FailoverRetCode retCode, const QString &hostname) {
        if (retCode == server_api::FailoverRetCode::kSuccess)
            qDebug() << "next hostname:" <<  hostname;
    });

    while (true) {
        qDebug() << "current hostname:" << failover->currentHostname();
        failover->getNextHostname(false);
        QSignalSpy spy(failover, SIGNAL(nextHostnameAnswer(server_api::FailoverRetCode, QString)));
        spy.wait(60000);
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        if (arguments.at(1).toString().isEmpty())
            break;
    };   
    qDebug() << "current hostname:" << failover->currentHostname();
}


QTEST_MAIN(TestFailover)
#include "failover.test.moc"
