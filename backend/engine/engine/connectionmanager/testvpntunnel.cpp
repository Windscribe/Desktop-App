#include "testvpntunnel.h"
#include "engine/serverapi/serverapi.h"
#include "utils/logger.h"
#include "utils/ipvalidation.h"

TestVPNTunnel::TestVPNTunnel(QObject *parent, ServerAPI *serverAPI) : QObject(parent),
    serverAPI_(serverAPI), bRunning_(false), curTest_(1), cmdId_(0)
{
    timeouts_ << PING_TEST_TIMEOUT_1 << PING_TEST_TIMEOUT_2 << PING_TEST_TIMEOUT_3;
    connect(serverAPI_, SIGNAL(pingTestAnswer(SERVER_API_RET_CODE,QString)), SLOT(onPingTestAnswer(SERVER_API_RET_CODE,QString)), Qt::QueuedConnection);
}

void TestVPNTunnel::startTests()
{
    // if tests already runned, stop it
    qCDebug(LOG_CONNECTION) << "TestVPNTunnel::startTests()";
    if (bRunning_)
    {
        qCDebug(LOG_CONNECTION) << "tests already running, stop previous";
        serverAPI_->cancelPingTest(cmdId_);
        bRunning_ = false;
    }

    // start first test
    qCDebug(LOG_CONNECTION) << "Doing tunnel test 1";
    bRunning_ = true;
    curTest_ = 1;
    elapsed_.start();
    cmdId_++;
    lastTimeForCallWithLog_ = QTime::currentTime();
    serverAPI_->pingTest(cmdId_, timeouts_[curTest_ - 1], true);
}

void TestVPNTunnel::stopTests()
{
    if (bRunning_)
    {
        qCDebug(LOG_CONNECTION) << "Tunnel tests stopped";
        bRunning_ = false;
        serverAPI_->cancelPingTest(cmdId_);
    }
}

void TestVPNTunnel::onPingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data)
{
    if (bRunning_)
    {
        const QString trimmedData = data.trimmed();
        if (retCode == SERVER_RETURN_SUCCESS && IpValidation::instance().isIp(trimmedData))
        {
            qCDebug(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "successfully finished with IP:" << trimmedData;
            bRunning_ = false;
            emit testsFinished(true, trimmedData);
        }
        else
        {
            if (elapsed_.elapsed() < timeouts_[curTest_-1])
            {
                // next ping attempt after 100 ms
                QTimer::singleShot(100, this, SLOT(doNextPingTest()));
            }
            else
            {
                qCDebug(LOG_CONNECTION) << "Tunnel test " << QString::number(curTest_) << "failed";

                if (curTest_ < 3)
                {
                    curTest_++;
                    elapsed_.start();
                    doNextPingTest();
                }
                else
                {
                    emit testsFinished(false, "");
                }
            }
        }
    }
}

void TestVPNTunnel::doNextPingTest()
{
    if (bRunning_ && curTest_ >= 1 && curTest_ <= 3)
    {
        // reduce log output (maximum 1 log output per 1 sec)
        bool bWriteLog = lastTimeForCallWithLog_.msecsTo(QTime::currentTime()) > 1000;
        if (bWriteLog)
        {
            lastTimeForCallWithLog_ = QTime::currentTime();
        }

        cmdId_++;
        if ((timeouts_[curTest_-1] - elapsed_.elapsed()) > 0)
        {
            serverAPI_->pingTest(cmdId_, timeouts_[curTest_-1] - elapsed_.elapsed(), bWriteLog);
        }
        else
        {
            serverAPI_->pingTest(cmdId_, 100, bWriteLog);
        }
    }
}
