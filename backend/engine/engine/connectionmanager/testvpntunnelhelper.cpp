#include "testvpntunnelhelper.h"
#include "engine/serverapi/serverapi.h"
#include "utils/logger.h"
#include "engine/crossplatformobjectfactory.h"
#include "engine/dnsresolver/dnsresolver.h"
#include "utils/ipvalidation.h"

quint64 TestVPNTunnelHelper::cmdId_ = 0;
QMutex TestVPNTunnelHelper::mutexCmdId_;

TestVPNTunnelHelper::TestVPNTunnelHelper(QObject *parent, ServerAPI *serverAPI) : QObject(parent), serverAPI_(serverAPI), state_(STATE_NONE),
    sentPingCmdId_(0), bStopped_(false), testNum_(0)
{
    connect(serverAPI_, SIGNAL(pingTestAnswer(SERVER_API_RET_CODE,QString)), SLOT(onPingTestAnswer(SERVER_API_RET_CODE,QString)), Qt::QueuedConnection);
}

TestVPNTunnelHelper::~TestVPNTunnelHelper()
{
    bStopped_ = true;
    serverAPI_->disconnect(this);
}

void TestVPNTunnelHelper::startTests(int testNum)
{
    qCDebug(LOG_CONNECTION) << "TestVPNTunnelHelper::startTests(" << testNum << ")";

    testNum_ = testNum;
    Q_ASSERT(state_ == STATE_NONE);

    if (!bStopped_)
    {
        mutexCmdId_.lock();
        sentPingCmdId_ = cmdId_;
        cmdId_++;
        mutexCmdId_.unlock();
        if (serverAPI_->isRequestsEnabled())
        {
            serverAPI_->pingTest(sentPingCmdId_, testNum_);
        }
        else
        {
            //skip ping test if serverAPI not ready
            onPingTestAnswer(SERVER_RETURN_SUCCESS, "Skip");
        }
        state_ = STATE_PING_REQUEST;
    }
}

void TestVPNTunnelHelper::stopTests()
{
    bStopped_ = true;
}

void TestVPNTunnelHelper::onPingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data)
{
    if (!bStopped_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            const QString trimmedData = data.trimmed();
            if (trimmedData == "Skip")
            {
                qCDebug(LOG_CONNECTION) << "Resolve test: ok;" << "Ping test: ok (skipped);" << "Fetch test: ok";
                state_ = STATE_NONE;
                emit testsFinished(true, testNum_, "");
            }
            else if (IpValidation::instance().isIp(trimmedData))
            {
                qCDebug(LOG_CONNECTION) << "Resolve test: ok; " << "Ping test: ok; " << "Fetch test: ok (" << trimmedData << ")";
                state_ = STATE_NONE;
                emit testsFinished(true, testNum_, trimmedData);
            }
            else
            {
                qCDebug(LOG_CONNECTION) << "Resolve test: ok;" << "Ping test: ok;" << "Fetch test: failed (incorrect answer from server" << data << ")";
                state_ = STATE_NONE;
                emit testsFinished(false, testNum_, "");
            }
        }
        else
        {
            qCDebug(LOG_CONNECTION) << "Resolve test: ok;" << "Ping test: ok;" << "Fetch test: failed (network error)";
            state_ = STATE_NONE;
            emit testsFinished(false, testNum_, "");
        }
    }
}
