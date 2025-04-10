#include "pingmultiplehosts.test.h"
#include <QtTest>
#include <QtConcurrent/QtConcurrent>

#ifdef Q_OS_WIN
    #include <WinSock2.h>
#endif

PingMultiplePings_test::PingMultiplePings_test()
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    // Load some addresses from a text file which contains a list of ips in the format (ip:hostname)
    testIps_ = readIps("test_ips.txt");

    // Some addresses that should ping with an error
    testFailedIps_ << qMakePair("23.23.23.23", "https://fr-stc-001.whiskergalaxy.com:6363/latency");
    testFailedIps_ << qMakePair("23.23.23.24", "https://fr-stc-001.whiskergalaxy.com:6363/latency");
    testFailedIps_ << qMakePair("23.23.23.25", "https://fr-stc-001.whiskergalaxy.com:6363/latency");

    accessManager_ = new NetworkAccessManager(this);
}

void PingMultiplePings_test::testBasic()
{
    PingMultipleHosts pingHosts(this, nullptr, accessManager_);

    //bool success, int timems, const QString &ip, bool isFromDisconnectedState)
    QSignalSpy spy(&pingHosts, &PingMultipleHosts::pingFinished);
    connect(&pingHosts, &PingMultipleHosts::pingFinished, [](bool success, int timems, const QString &ip, bool isFromDisconnectedState) {
        qDebug() << ip << success << timems;
    });

    PingType pt = PingType::kTcp;
    for (const auto &ip : std::as_const(testIps_)) {
        pingHosts.addHostForPing(ip.first, ip.second, pt);
        pt = getNextPingType(pt);
    }

    for (const auto &ip : std::as_const(testFailedIps_)) {
        pingHosts.addHostForPing(ip.first, ip.second, PingType::kIcmp);
    }

    for (const auto &ip : std::as_const(testIps_)) {
        pingHosts.addHostForPing(ip.first, ip.second, PingType::kIcmp);
    }

    while (spy.count() < (testIps_.count() + testFailedIps_.count()))
        spy.wait(30000);
}

QList<QPair<QString, QString>> PingMultiplePings_test::readIps(const QString &filename)
{
    QList<QPair<QString, QString>> ips;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("//"))
                continue;
            QStringList pars = line.split(";");
            Q_ASSERT(pars.size() == 2);
            ips << qMakePair(pars[0].trimmed(), pars[1].trimmed());
        }

    } else {
        Q_ASSERT(false);
    }
    return ips;
}

QTEST_MAIN(PingMultiplePings_test)
