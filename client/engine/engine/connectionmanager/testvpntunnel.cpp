#include "testvpntunnel.h"
#include "utils/log/categories.h"
#include "utils/ipvalidation.h"
#include "utils/extraconfig.h"
#include "utils/ws_assert.h"

using namespace wsnet;

TestVPNTunnel::TestVPNTunnel(QObject *parent) : QObject(parent),
    bRunning_(false), curTest_(1), doCustomTunnelTest_(false)
{
}

TestVPNTunnel::~TestVPNTunnel()
{
    if (curRequest_)
        curRequest_->cancel();
}

void TestVPNTunnel::startTests(const types::Protocol &protocol)
{
    qCDebug(LOG_CONNECTION) << "TestVPNTunnel::startTests()";

    stopTests();

    protocol_ = protocol;

    bool advParamExists;
    int delay = ExtraConfig::instance().getTunnelTestStartDelay(advParamExists);

    if (advParamExists)
    {
        qCInfo(LOG_CONNECTION) << "Delaying tunnel test start for" << delay << "ms";
        QTimer::singleShot(delay, this, &TestVPNTunnel::startTestImpl);
    }
    else {
        startTestImpl();
    }
}

void TestVPNTunnel::startTestImpl()
{
    timeouts_.clear();

    doCustomTunnelTest_ = false;

    bool advParamExists;
    int attempts = ExtraConfig::instance().getTunnelTestAttempts(advParamExists);
    if (advParamExists) {
        if (attempts == 0) {
            // Do not emit result directly here, callers may not be ready for callback before startTests() returns.
            QTimer::singleShot(1, this, &TestVPNTunnel::onTestsSkipped);
            return;
        }
        doCustomTunnelTest_ = true;
    }

    int timeout = ExtraConfig::instance().getTunnelTestTimeout(advParamExists);
    if (advParamExists) {
        doCustomTunnelTest_ = true;
        timeouts_.fill(timeout, attempts);
    }
    else {
        // There is a delay when using OpenVPN+wintun preventing DNS resolution once the tunnel is up.
        // The delay also occurs when using the vanilla OpenVPN client.  We tried experimenting with
        // the various settings for the --ip-win32 OpenVPN parameter, but none helped.  Increasing the
        // initial tunnel test timeout appears to dramatically improve the tunnel test time.  The test
        // was taking ~5.5s before the change, and is now taking ~3-3.5s.  Verified on x86_64 and arm64
        // Windows 10/11.
        if (protocol_.isOpenVpnProtocol()) {
            timeouts_ << 5000;
            timeouts_ << 10000;
            timeouts_ << 15000;
        }
        else {
            timeouts_ << 2000;
            timeouts_ << 4000;
            timeouts_ << 8000;
        }
    }

    testRetryDelay_ = ExtraConfig::instance().getTunnelTestRetryDelay(advParamExists);
    if (!advParamExists) {
        testRetryDelay_ = 0;
    } else {
        doCustomTunnelTest_ = true;
    }

    if (doCustomTunnelTest_) {
        qCInfo(LOG_CONNECTION) << "Running custom tunnel test with" << attempts << "attempts, timeout of" << timeout << "ms, and retry delay of" << testRetryDelay_ << "ms";
    }

    // start first test
    qCInfo(LOG_CONNECTION) << "Doing tunnel test 1";
    bRunning_ = true;
    curTest_ = 1;
    elapsed_.start();
    elapsedOverallTimer_.start();
    lastTimeForCallWithLog_ = QTime::currentTime();

    WS_ASSERT(curRequest_ == nullptr);
    curRequest_ = callPingTest(timeouts_[curTest_ - 1]);
}

void TestVPNTunnel::stopTests()
{
    if (bRunning_) {
        bRunning_ = false;
        if (curRequest_) {
            curRequest_->cancel();
            curRequest_.reset();
        }
        qCInfo(LOG_CONNECTION) << "Tunnel tests stopped";
    }
}

void TestVPNTunnel::onPingTestAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &ipAddress)
{
    WS_ASSERT(curRequest_ != nullptr);
    curRequest_.reset();

    if (bRunning_) {
        const QString trimmedData = QString::fromStdString(ipAddress).trimmed();
        if (serverApiRetCode == ServerApiRetCode::kSuccess && IpValidation::isIp(trimmedData)) {
            qCInfo(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "successfully finished with IP:" << trimmedData << ", total test time =" << elapsedOverallTimer_.elapsed();
            bRunning_ = false;
            emit testsFinished(true, trimmedData);
        } else {
            if (doCustomTunnelTest_) {
                qCInfo(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "failed";

                if (curTest_ < timeouts_.size()) {
                    curTest_++;
                    QTimer::singleShot(testRetryDelay_, this, &TestVPNTunnel::doNextPingTest);
                } else {
                    bRunning_ = false;
                    emit testsFinished(false, "");
                }
            } else {
                if (elapsed_.elapsed() < timeouts_[curTest_-1]) {
                    // next ping attempt after 100 ms
                    QTimer::singleShot(100, this, &TestVPNTunnel::doNextPingTest);
                } else {
                    qCInfo(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "failed";

                    if (curTest_ < timeouts_.size()) {
                        curTest_++;
                        elapsed_.start();
                        doNextPingTest();
                    } else {
                        bRunning_ = false;
                        emit testsFinished(false, "");
                    }
                }
            }
        }
    }
}

void TestVPNTunnel::doNextPingTest()
{
    if (bRunning_ && curTest_ >= 1 && curTest_ <= timeouts_.size()) {
        WS_ASSERT(curRequest_ == nullptr);

        if (doCustomTunnelTest_) {
            curRequest_ = callPingTest(timeouts_[curTest_ - 1]);
        } else {
            // reduce log output (maximum 1 log output per 1 sec)
            bool bWriteLog = lastTimeForCallWithLog_.msecsTo(QTime::currentTime()) > 1000;
            if (bWriteLog) {
                lastTimeForCallWithLog_ = QTime::currentTime();
            }
            if ((timeouts_[curTest_-1] - elapsed_.elapsed()) > 0) {
                curRequest_ = callPingTest(timeouts_[curTest_-1] - elapsed_.elapsed());
            } else {
                curRequest_ = callPingTest(100);
            }
        }
    }
}


void TestVPNTunnel::onTestsSkipped()
{
    qCDebug(LOG_CONNECTION) << "Tunnel tests disabled";
    emit testsFinished(true, "");
}

std::shared_ptr<WSNetCancelableCallback> TestVPNTunnel::callPingTest(std::uint32_t timeoutMs)
{
    auto request = WSNet::instance()->serverAPI()->pingTest(timeoutMs, [this](wsnet::ServerApiRetCode serverApiRetCode, const std::string &ipAddress)
    {
        // put in message loop
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, ipAddress]() { // NOLINT: false positive for memory leak
            onPingTestAnswer(serverApiRetCode, ipAddress);
        });
    });
    return request;
}
