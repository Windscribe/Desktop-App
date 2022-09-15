#include <QtTest>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <WinSock2.h>
#include <QDnsLookup>
#include "engine/dnsresolver/dnsrequest.h"
#include "utils/utils.h"

class TestDnsRequest : public QObject
{
    Q_OBJECT

public:
    TestDnsRequest();
    ~TestDnsRequest();


private slots:
    void test_async();
    void test_blocked();
    void test_incorrect();
    void test_subdomain();
    void test_timeout();
    void test_timeout_blocked();

private:
    QStringList domains_;
    QMutex mutex_;

    QString getRandomDomain();
};


TestDnsRequest::TestDnsRequest()
{
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    QFile file("test_domains.txt");
    file.open(QIODevice::ReadOnly);
    QVERIFY(file.isOpen());

    QTextStream in(&file);
    while (!in.atEnd())
        domains_ << in.readLine();
}

TestDnsRequest::~TestDnsRequest()
{

}

void TestDnsRequest::test_async()
{
    // run 10 requests async
    QList< QFuture< void > > futures;

    for (int i = 0; i < 10; ++i) {
        QFuture<void> future = QtConcurrent::run([=]()
        {
            DnsRequest *request = new DnsRequest(NULL, getRandomDomain(), QStringList());
            //DnsRequest *request = new DnsRequest(NULL, getRandomDomain(), QStringList() << "34.23.56.78" << "83.82.81.18");

            connect(
                request, &DnsRequest::finished,
                [request]()
            {
                request->deleteLater();
            });

            QSignalSpy spySignal(request, SIGNAL(destroyed()));
            request->lookup();

            spySignal.wait(20000);
            QCOMPARE(spySignal.count(), 1);
        });

        futures << future;
    }

    // test delete requests
    for (int i = 0; i < 10; ++i) {
        DnsRequest *request = new DnsRequest(NULL, getRandomDomain());
        request->lookup();
        request->deleteLater();
    }

    for (auto it : futures) {
        it.waitForFinished();
    }
}


void TestDnsRequest::test_blocked()
{
    DnsRequest *request = new DnsRequest(this, "1.2.3.4");
    request->lookupBlocked();
    QCOMPARE(request->isError(), false);
    QCOMPARE(request->ips().count(), 1);
    QCOMPARE(request->ips()[0], "1.2.3.4");
}

void TestDnsRequest::test_incorrect()
{
    DnsRequest *request = new DnsRequest(this, "incorrectdomain");
    request->lookupBlocked();
    QCOMPARE(request->isError(), true);
    QCOMPARE(request->ips().count(), 0);

    DnsRequest *request2 = new DnsRequest(this, "192.126.1.256");
    request2->lookupBlocked();
    QCOMPARE(request2->isError(), true);
    QCOMPARE(request2->ips().count(), 0);
}

void TestDnsRequest::test_subdomain()
{
    DnsRequest *request = new DnsRequest(this, "blog.hubspot.com");
    request->lookupBlocked();
    QCOMPARE(request->isError(), false);
    QVERIFY(request->ips().count() != 0);
}

void TestDnsRequest::test_timeout()
{
    QElapsedTimer timer;
    timer.start();

    DnsRequest *request = new DnsRequest(this, "google.com", QStringList() << "192.0.2.10", 2000);
    request->lookup();

    QSignalSpy spySignal(request, SIGNAL(finished()));
    spySignal.wait(10000);
    QCOMPARE(spySignal.count(), 1);

    QVERIFY(timer.elapsed() >= 1800 && timer.elapsed() <= 2200);
}

void TestDnsRequest::test_timeout_blocked()
{
    QElapsedTimer timer;
    timer.start();
    DnsRequest *request = new DnsRequest(this, "google.com", QStringList() << "192.0.2.10", 2000);
    request->lookupBlocked();
    QVERIFY(timer.elapsed() >= 1800 && timer.elapsed() <= 2200);
}


QString TestDnsRequest::getRandomDomain()
{
    QMutexLocker locker(&mutex_);
    return domains_[Utils::generateIntegerRandom(10, domains_.size() - 1)];

}


QTEST_MAIN(TestDnsRequest)
#include "dnsrequest.test.moc"
