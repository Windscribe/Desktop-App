#include "testvpntunnel.h"
#include "engine/serverapi/serverapi.h"
#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "utils/extraconfig.h"

#if defined(Q_OS_WINDOWS)
#include "utils/hardcodedsettings.h"
#include "../openvpnversioncontroller.h"
#endif

#if defined(Q_OS_WINDOWS)
class DnsQueryContext
{
public:
    explicit DnsQueryContext();
    ~DnsQueryContext();

    bool initialize();

    DNS_QUERY_RESULT* queryResult() const { return result_; }
    DNS_QUERY_CANCEL* cancelHandle() const { return cancelHandle_; }
    bool canCancel() const { return cancelHandle_ != NULL; }
    DNS_STATUS queryStatus() const;

    QStringList ips() const;

private:
    DNS_QUERY_RESULT* result_;
    DNS_QUERY_CANCEL* cancelHandle_;

    void release();
};

DnsQueryContext::DnsQueryContext() : result_(NULL), cancelHandle_(NULL)
{
}

DnsQueryContext::~DnsQueryContext()
{
    release();
}

bool DnsQueryContext::initialize()
{
    release();

    // Due to the alignment spec of DNS_QUERY_CANCEL in WinDNS.h, we need to use the Win32 heap
    // rather than using new.  I was getting memory corruption issues in the class instance
    // immediately after using new to allocate the DNS_QUERY_CANCEL structure.

    result_ = (DNS_QUERY_RESULT*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DNS_QUERY_RESULT));
    if (result_ == NULL)
    {
        qCDebug(LOG_CONNECTION) << "TestVPNTunnel DNS_QUERY_RESULT allocation failed:" << ::GetLastError();
        return false;
    }

    result_->Version = DNS_QUERY_REQUEST_VERSION1;

    cancelHandle_ = (DNS_QUERY_CANCEL*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DNS_QUERY_CANCEL));
    if (cancelHandle_ == NULL)
    {
        qCDebug(LOG_CONNECTION) << "TestVPNTunnel DNS_QUERY_CANCEL allocation failed:" << ::GetLastError();
        return false;
    }

    return true;
}

void DnsQueryContext::release()
{
    if (result_ != NULL)
    {
        if (result_->pQueryRecords != NULL) {
            DnsRecordListFree(result_->pQueryRecords, DnsFreeRecordList);
        }

        ::HeapFree(::GetProcessHeap(), 0, result_);
        result_ = NULL;
    }

    if (cancelHandle_ != NULL)
    {
        ::HeapFree(::GetProcessHeap(), 0, cancelHandle_);
        cancelHandle_ = NULL;
    }
}

DNS_STATUS DnsQueryContext::queryStatus() const
{
    if (result_ != NULL) {
        return result_->QueryStatus;
    }

    return ERROR_INVALID_DATA;
}

QStringList DnsQueryContext::ips() const
{
    QStringList ips;

    if (result_ != NULL)
    {
        PDNS_RECORD pDnsRecord = result_->pQueryRecords;
        while (pDnsRecord && pDnsRecord->wType == DNS_TYPE_A)
        {
            QHostAddress addr(ntohl(pDnsRecord->Data.A.IpAddress));
            ips.append(addr.toString());
            pDnsRecord = pDnsRecord->pNext;
        }
    }

    return ips;
}

#endif

TestVPNTunnel::TestVPNTunnel(QObject *parent, ServerAPI *serverAPI) : QObject(parent),
    serverAPI_(serverAPI), bRunning_(false), curTest_(1), cmdId_(0), doCustomTunnelTest_(false), doWin32TunnelTest_(false)
{
#if defined(Q_OS_WINDOWS)
    dllHandle_ = NULL;
    DnsQueryEx_f = NULL;
    DnsCancelQuery_f = NULL;

    dnsQueryTimeout_.setSingleShot(true);
    dnsQueryTimeout_.setInterval(PING_TEST_TIMEOUT_1 + PING_TEST_TIMEOUT_2 + PING_TEST_TIMEOUT_3);
    connect(&dnsQueryTimeout_, &QTimer::timeout, this, &TestVPNTunnel::onWin32DnsQueryTimeout);

#endif
    connect(serverAPI_, &ServerAPI::pingTestAnswer, this, &TestVPNTunnel::onPingTestAnswer, Qt::QueuedConnection);
}

TestVPNTunnel::~TestVPNTunnel()
{
    #if defined(Q_OS_WINDOWS)
    if (dllHandle_ != NULL) {
        ::FreeLibrary(dllHandle_);
    }
    #endif
}

void TestVPNTunnel::startTests(const ProtocolType &protocol)
{
    qCDebug(LOG_CONNECTION) << "TestVPNTunnel::startTests()";

    stopTests();

    protocol_ = protocol;

    #if defined(Q_OS_WINDOWS)
    doWin32TunnelTest_ = protocol.isWireGuardProtocol() || (protocol.isOpenVpnProtocol() && OpenVpnVersionController::instance().isUseWinTun());
    if (doWin32TunnelTest_)
    {
        serverTunnelTestUrl_ = HardcodedSettings::instance().serverTunnelTestUrl().toStdWString();

        if (loadWinDnsApiEndpoints() && initiateWin32TunnelTest())
        {
            bRunning_ = true;
            dnsQueryTimeout_.start();
            qCDebug(LOG_CONNECTION) << "TestVPNTunnel DnsQueryEx initiated to" << QString::fromStdWString(serverTunnelTestUrl_);
            return;
        }

        doWin32TunnelTest_ = false;
    }
    #endif

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
    cmdId_++;
    lastTimeForCallWithLog_ = QTime::currentTime();
    serverAPI_->pingTest(cmdId_, timeouts_[curTest_ - 1], true);
}

void TestVPNTunnel::stopTests()
{
    if (bRunning_)
    {
        bRunning_ = false;

        if (doWin32TunnelTest_)
        {
            doWin32TunnelTest_ = false;

            #if defined(Q_OS_WINDOWS)
            dnsQueryTimeout_.stop();

            if (!dnsQueryContext_.isNull() && dnsQueryContext_->canCancel()) {
                DnsCancelQuery_f(dnsQueryContext_->cancelHandle());
            }
            #endif
        }
        else {
            serverAPI_->cancelPingTest(cmdId_);
        }

        qCDebug(LOG_CONNECTION) << "Tunnel tests stopped";
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
            doWin32TunnelTest_ = false;

            #if defined(Q_OS_WINDOWS)
            dnsQueryTimeout_.stop();
            #endif

            emit testsFinished(true, trimmedData);
        }
        else
        {
            if (doWin32TunnelTest_)
            {
                bRunning_ = false;
                doWin32TunnelTest_ = false;

                #if defined(Q_OS_WINDOWS)
                dnsQueryTimeout_.stop();
                #endif

                emit testsFinished(true, trimmedData);

                return;
            }

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
        cmdId_++;

        if (doCustomTunnelTest_)
        {
            serverAPI_->pingTest(cmdId_, timeouts_[curTest_-1], true);
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
                serverAPI_->pingTest(cmdId_, timeouts_[curTest_-1] - elapsed_.elapsed(), bWriteLog);
            }
            else
            {
                serverAPI_->pingTest(cmdId_, 100, bWriteLog);
            }
        }
    }
}

#if defined(Q_OS_WINDOWS)
bool TestVPNTunnel::loadWinDnsApiEndpoints()
{
    if (dllHandle_ == NULL)
    {
        dllHandle_ = ::LoadLibraryW(L"Dnsapi.dll");
        if (dllHandle_ == NULL)
        {
            qCDebug(LOG_CONNECTION) << "TestVPNTunnel failed to load Dnsapi.dll:" << ::GetLastError();
            return false;
        }
    }

    // DnsQueryEx is only available on Win8 and newer.  GetProcAddress will return NULL on Win7, causing
    // us to failover to the old tunnel test methodology.
    if (DnsQueryEx_f == NULL)
    {
        DnsQueryEx_f = (DnsQueryEx_T*)::GetProcAddress(dllHandle_, "DnsQueryEx");

        if (DnsQueryEx_f == NULL)
        {
            qCDebug(LOG_CONNECTION) << "TestVPNTunnel failed to load DnsQueryEx:" << ::GetLastError();
            ::FreeLibrary(dllHandle_);
            dllHandle_ = NULL;
            return false;
        }
    }

    if (DnsCancelQuery_f == NULL)
    {
        DnsCancelQuery_f = (DnsCancelQuery_T*)::GetProcAddress(dllHandle_, "DnsCancelQuery");

        if (DnsCancelQuery_f == NULL)
        {
            qCDebug(LOG_CONNECTION) << "TestVPNTunnel failed to load DnsCancelQuery:" << ::GetLastError();
            ::FreeLibrary(dllHandle_);
            dllHandle_ = NULL;
            DnsQueryEx_f = NULL;
            return false;
        }
    }

    return true;
}

VOID WINAPI
TestVPNTunnel::DnsQueryCompleteCallback(_In_ PVOID Context, _Inout_ PDNS_QUERY_RESULT QueryResults)
{
    Q_UNUSED(QueryResults)

    // Warning: this method is called by an OS worker thread.

    TestVPNTunnel* pThis = static_cast< TestVPNTunnel* >(Context);
    if (pThis)
    {
        bool invoked = QMetaObject::invokeMethod(pThis, &TestVPNTunnel::onWin32DnsQueryCompleted, Qt::QueuedConnection);
        if (!invoked) {
            qCDebug(LOG_CONNECTION) << "TestVPNTunnel could not invoke OnWin32DnsQueryCompleted";
        }
    }
}

bool TestVPNTunnel::initiateWin32TunnelTest()
{
    if (!doWin32TunnelTest_) {
        return false;
    }

    Q_ASSERT(!serverTunnelTestUrl_.empty());

    DNS_QUERY_REQUEST request;
    ::ZeroMemory(&request, sizeof(request));
    request.Version        = DNS_QUERY_REQUEST_VERSION1;
    request.QueryName      = serverTunnelTestUrl_.c_str();
    request.QueryType      = DNS_TYPE_A;
    request.pQueryContext  = this;
    request.pQueryCompletionCallback = DnsQueryCompleteCallback;

    if (protocol_.isOpenVpnProtocol() && OpenVpnVersionController::instance().isUseWinTun())
    {
        // Using these options due to a delay issue with OpenVPN+Wintun.  There is a period of
        // time when the tunnel first comes up in which DNS queries will not be passed over the
        // tunnel.  The default options use UDP, which will cause the completion callback to
        // eventually be invoked after ~10s with ERROR_TIMEOUT.  Using TCP, the callback is
        // invoked almost immediately with WSAENOTCONN.
        // However, if the DNS server does not respond to TCP requests, the tunnel tests
        // may have a false positive where the test fails but the tunnel generally works fine.
        request.QueryOptions = DNS_QUERY_USE_TCP_ONLY | DNS_QUERY_WIRE_ONLY;
    }

    dnsQueryContext_.reset(new DnsQueryContext);

    if (dnsQueryContext_.isNull() || !dnsQueryContext_->initialize()) {
        return false;
    }

    DNS_STATUS result = DnsQueryEx_f(&request, dnsQueryContext_->queryResult(), dnsQueryContext_->cancelHandle());

    if (result != DNS_REQUEST_PENDING)
    {
        qCDebug(LOG_CONNECTION) << "TestVPNTunnel DnsQueryEx initiation failed:" << result;
        return false;
    }

    return true;
}

void TestVPNTunnel::onWin32DnsQueryCompleted()
{
    if (!doWin32TunnelTest_)
    {
        dnsQueryContext_.reset();
        return;
    }

    if (dnsQueryContext_->queryStatus() == ERROR_SUCCESS)
    {
        QStringList ips = dnsQueryContext_->ips();
        dnsQueryContext_.reset();
        qCDebug(LOG_CONNECTION) << "TestVPNTunnel DnsQueryEx successful with IPs:" << ips;
        serverAPI_->onTunnelTestDnsResolve(ips);
    }
    else
    {
        // WSAENOTCONN indicates OpenVPN and/or Wintun are not quite ready yet to accept
        // our DNS request.  We can thus be a bit more aggressive with our retries.
        int timeout = 100;
        if (dnsQueryContext_->queryStatus() != WSAENOTCONN)
        {
            qCDebug(LOG_CONNECTION) << "TestVPNTunnel DnsQueryEx failed:" << dnsQueryContext_->queryStatus();
            timeout = 1000;
        }

        QTimer::singleShot(timeout, [this]() {
            if (!initiateWin32TunnelTest())
            {
                bRunning_ = false;
                dnsQueryTimeout_.stop();

                if (doWin32TunnelTest_)
                {
                    // We haven't been told to stop, so let the upper levels know the tunnel test failed.
                    doWin32TunnelTest_ = false;
                    emit testsFinished(false, "");
                }

                dnsQueryContext_.reset();
            }
        });
    }
}

void TestVPNTunnel::onWin32DnsQueryTimeout()
{
    bRunning_ = false;

    if (doWin32TunnelTest_)
    {
        doWin32TunnelTest_ = false;
        emit testsFinished(false, "");
    }

    if (!dnsQueryContext_.isNull() && dnsQueryContext_->canCancel()) {
        DnsCancelQuery_f(dnsQueryContext_->cancelHandle());
    }

    qCDebug(LOG_CONNECTION) << "TestVPNTunnel WinDns query timed out";
}

#endif

void TestVPNTunnel::onTestsSkipped()
{
    qCDebug(LOG_CONNECTION) << "Tunnel tests disabled";
    emit testsFinished(true, "");
}

