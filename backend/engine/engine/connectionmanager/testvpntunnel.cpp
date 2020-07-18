#include "testvpntunnel.h"
#include "Engine/ServerApi/serverapi.h"
#include "testvpntunnelhelper.h"
#include "Utils/logger.h"


TestVPNTunnel::TestVPNTunnel(QObject *parent, ServerAPI *serverAPI) : QObject(parent), bRunning_(false), testVPNTunnelHelper_(NULL),
    serverAPI_(serverAPI)
{
    startTestTimer_.setInterval(500);
    startTestTimer_.setSingleShot(true);
    connect(&startTestTimer_, SIGNAL(timeout()), SLOT(onStartTestTimerTick()));
}

TestVPNTunnel::~TestVPNTunnel()
{
    startTestTimer_.stop();
    deleteTestVpnTunnelHelper();
}

void TestVPNTunnel::startTests()
{
    // if tests already runned, stop it
    qCDebug(LOG_CONNECTION) << "TestVPNTunnel::startTests()";
    if (bRunning_)
    {
        qCDebug(LOG_CONNECTION) << "tests already running, stop previous";
        startTestTimer_.stop();
        deleteTestVpnTunnelHelper();
    }

    bRunning_ = true;
    startTestTimer_.start();
}

void TestVPNTunnel::stopTests()
{
    if (bRunning_)
    {
        qCDebug(LOG_CONNECTION) << "TestVPNTunnel::stopTests()";
        startTestTimer_.stop();
        deleteTestVpnTunnelHelper();
        bRunning_ = false;
    }
}

void TestVPNTunnel::onHelperTestsFinished(bool bSuccess, int testNum, const QString & ipAddress)
{
    if (!bRunning_)
    {
        return;
    }

    startTestTimer_.stop();
    if (bSuccess)
    {
        stopTests();
        emit testsFinished(true, ipAddress);
    }
    else
    {
        if (testNum == 1)
        {
            // do second test
            qCDebug(LOG_CONNECTION) << "First test tunnel failed";
            deleteTestVpnTunnelHelper();
            startHelperTest(TIMER_JOB_TEST2);
        }
        else if (testNum == 2)
        {
            // do third test
            qCDebug(LOG_CONNECTION) << "Second test tunnel failed";
            deleteTestVpnTunnelHelper();
            startHelperTest(TIMER_JOB_TEST3);
        }
        else
        {
            qCDebug(LOG_CONNECTION) << "Third test tunnel failed";
            stopTests();
            emit testsFinished(false,"");
        }
    }
}

void TestVPNTunnel::onStartTestTimerTick()
{
    startHelperTest(TIMER_JOB_TEST1);
}

void TestVPNTunnel::startHelperTest(int testNum)
{
    Q_ASSERT(testVPNTunnelHelper_ == NULL);
    testVPNTunnelHelper_ = new TestVPNTunnelHelper(this, serverAPI_);
    connect(testVPNTunnelHelper_, SIGNAL(testsFinished(bool, int, QString)), SLOT(onHelperTestsFinished(bool, int, QString)));
    testVPNTunnelHelper_->startTests(testNum);
}

void TestVPNTunnel::deleteTestVpnTunnelHelper()
{
    if (testVPNTunnelHelper_)
    {
        testVPNTunnelHelper_->disconnect(this);
        testVPNTunnelHelper_->stopTests();
        testVPNTunnelHelper_->deleteLater();
        testVPNTunnelHelper_ = NULL;
    }
}
