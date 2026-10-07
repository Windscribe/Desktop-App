#include "testvpntunnel.h"
#include "utils/log/categories.h"

TestVPNTunnel::TestVPNTunnel(QObject *parent) : QObject(parent)
{
}

TestVPNTunnel::~TestVPNTunnel()
{
    if (curRequest_)
        curRequest_->cancel();
}

void TestVPNTunnel::startTests(bool isBridgeAllowed)
{
    qCDebug(LOG_CONNECTION) << "TestVPNTunnel::startTests()";

    stopTests();

    const quint64 runId = ++runId_;
    curRequest_ = wsnet::WSNet::instance()->tunnelTest()->start(isBridgeAllowed,
        [this, runId](bool isSuccess, const std::string &ipv4, const std::string &ipv6)
    {
        // put in message loop
        QMetaObject::invokeMethod(this, [this, runId, isSuccess, ipv4, ipv6]() { // NOLINT: false positive for memory leak
            onTestFinished(runId, isSuccess, ipv4, ipv6);
        });
    });
}

void TestVPNTunnel::stopTests()
{
    if (curRequest_) {
        runId_++;
        curRequest_->cancel();
        curRequest_.reset();
        qCInfo(LOG_CONNECTION) << "Tunnel tests stopped";
    }
}

void TestVPNTunnel::onTestFinished(quint64 runId, bool isSuccess, const std::string &ipv4, const std::string &ipv6)
{
    if (runId != runId_) {
        return;
    }
    curRequest_.reset();
    // The frontends display a single address, the same family my_ip reports; ipv6 stands in only when
    // the node answered without an ipv4.
    emit testsFinished(isSuccess, QString::fromStdString(ipv4.empty() ? ipv6 : ipv4));
}
