#include "testvpntunnel.h"
#include "engine/serverapi/serverapi.h"
#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "utils/extraconfig.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "engine/serverapi/requests/pingtestrequest.h"


TestVPNTunnel::TestVPNTunnel(QObject *parent, server_api::ServerAPI *serverAPI) : QObject(parent),
    serverAPI_(serverAPI), bRunning_(false), curTest_(1), cmdId_(0), doCustomTunnelTest_(false), curRequest_(nullptr)
{
}

TestVPNTunnel::~TestVPNTunnel()
{
}

void TestVPNTunnel::startTests(const PROTOCOL &protocol)
{
    qCDebug(LOG_CONNECTION) << "TestVPNTunnel::startTests()";

    stopTests();

    protocol_ = protocol;

    bool advParamExists;
    int delay = ExtraConfig::instance().getTunnelTestStartDelay(advParamExists);

    if (advParamExists)
    {
        qCDebug(LOG_CONNECTION) << "Delaying tunnel test start for" << delay << "ms";
        QTimer::singleShot(delay, this, &TestVPNTunnel::startTestImpl);
    }
    else {
        startTestImpl();
    }
}

void TestVPNTunnel::startTestImpl()
{
    timeouts_.clear();

    bool advParamExists;

    int attempts = ExtraConfig::instance().getTunnelTestAttempts(advParamExists);
    if (!advParamExists) {
        attempts = 3;
    } else {
        doCustomTunnelTest_ = true;
    }
    if (attempts == 0) {
        // Do not emit result directly here, callers may not be ready for callback before startTests() returns.
        QTimer::singleShot(1, this, &TestVPNTunnel::onTestsSkipped);
        return;
    }

    int timeout = ExtraConfig::instance().getTunnelTestTimeout(advParamExists);
    if (!advParamExists) {
        int timeout = PING_TEST_TIMEOUT_1;
        for (int i = 0; i < attempts; i++)
        {
            timeouts_ << timeout;
            if (timeout < PING_TEST_TIMEOUT_3)
            {
                timeout *= 2;
            }
        }
    } else {
        doCustomTunnelTest_ = true;
        timeouts_.fill(timeout, attempts);
    }

    testRetryDelay_ = ExtraConfig::instance().getTunnelTestRetryDelay(advParamExists);
    if (!advParamExists) {
        testRetryDelay_ = 0;
    } else {
        doCustomTunnelTest_ = true;
    }

    if (doCustomTunnelTest_) {
        qCDebug(LOG_CONNECTION) << "Running custom tunnel test with" << attempts << "attempts, timeout of" << timeout << "ms, and retry delay of" << testRetryDelay_ << "ms";
    }

    // start first test
    qCDebug(LOG_CONNECTION) << "Doing tunnel test 1";
    bRunning_ = true;
    curTest_ = 1;
    elapsed_.start();
    elapsedOverallTimer_.start();
    cmdId_++;
    lastTimeForCallWithLog_ = QTime::currentTime();

    WS_ASSERT(curRequest_ == nullptr);
    curRequest_ = serverAPI_->pingTest(timeouts_[curTest_ - 1], true);
    connect(curRequest_, &server_api::BaseRequest::finished, this, &TestVPNTunnel::onPingTestAnswer);
}

void TestVPNTunnel::stopTests()
{
    if (bRunning_)
    {
        bRunning_ = false;
        SAFE_DELETE(curRequest_);
        qCDebug(LOG_CONNECTION) << "Tunnel tests stopped";
    }
}

void TestVPNTunnel::onPingTestAnswer()
{
    QSharedPointer<server_api::PingTestRequest> request(static_cast<server_api::PingTestRequest *>(sender()), &QObject::deleteLater);
    WS_ASSERT(curRequest_ != nullptr);
    curRequest_ = nullptr;

    if (bRunning_)
    {
        const QString trimmedData = request->data().trimmed();
        if (request->retCode() == SERVER_RETURN_SUCCESS && IpValidation::instance().isIp(trimmedData))
        {
            qCDebug(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "successfully finished with IP:" << trimmedData << ", total test time =" << elapsedOverallTimer_.elapsed();
            bRunning_ = false;
            emit testsFinished(true, trimmedData);
        }
        else
        {
            if (doCustomTunnelTest_)
            {
                qCDebug(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "failed";

                if (curTest_ < timeouts_.size())
                {
                    curTest_++;
                    QTimer::singleShot(testRetryDelay_, this, &TestVPNTunnel::doNextPingTest);
                }
                else
                {
                    bRunning_ = false;
                    emit testsFinished(false, "");
                }
            }
            else
            {
                if (elapsed_.elapsed() < timeouts_[curTest_-1])
                {
                    // next ping attempt after 100 ms
                    QTimer::singleShot(100, this, &TestVPNTunnel::doNextPingTest);
                }
                else
                {
                    qCDebug(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "failed";

                    if (curTest_ < timeouts_.size())
                    {
                        curTest_++;
                        elapsed_.start();
                        doNextPingTest();
                    }
                    else
                    {
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
    if (bRunning_ && curTest_ >= 1 && curTest_ <= timeouts_.size())
    {
        WS_ASSERT(curRequest_ == nullptr);

        cmdId_++;

        if (doCustomTunnelTest_)
        {
            curRequest_ = serverAPI_->pingTest(timeouts_[curTest_ - 1], true);
            connect(curRequest_, &server_api::BaseRequest::finished, this, &TestVPNTunnel::onPingTestAnswer);
        }
        else
        {
            // reduce log output (maximum 1 log output per 1 sec)
            bool bWriteLog = lastTimeForCallWithLog_.msecsTo(QTime::currentTime()) > 1000;
            if (bWriteLog)
            {
                lastTimeForCallWithLog_ = QTime::currentTime();
            }

            if ((timeouts_[curTest_-1] - elapsed_.elapsed()) > 0)
            {
                curRequest_ = serverAPI_->pingTest(timeouts_[curTest_-1] - elapsed_.elapsed(), bWriteLog);
                connect(curRequest_, &server_api::BaseRequest::finished, this, &TestVPNTunnel::onPingTestAnswer);
            }
            else
            {
                curRequest_ = serverAPI_->pingTest(100, bWriteLog);
                connect(curRequest_, &server_api::BaseRequest::finished, this, &TestVPNTunnel::onPingTestAnswer);
            }
        }
    }
}


void TestVPNTunnel::onTestsSkipped()
{
    qCDebug(LOG_CONNECTION) << "Tunnel tests disabled";
    emit testsFinished(true, "");
}

