#include "dnsresolver_win.h"
#include "utils/logger.h"

#include <windows.h>
#include <windns.h>
#include <Ws2tcpip.h>
#include <Mstcpip.h>
#include <QElapsedTimer>

#include "utils/ws_assert.h"
#include "utils/timer_win.h"

// structure passed to queryCompleteCallback
struct DnsQueryInfo
{
    QSharedPointer<QObject> object;
    DNS_QUERY_RESULT dnsQueryResult;
    DNS_QUERY_CANCEL dnsQueryCancel;
    wsl::Win32Handle timer;
    QElapsedTimer elapsed;


    DnsQueryInfo()
    {
        ZeroMemory(&dnsQueryResult, sizeof(dnsQueryResult));
        dnsQueryResult.Version = DNS_QUERY_RESULTS_VERSION1;

        ZeroMemory(&dnsQueryCancel, sizeof(dnsQueryCancel));
    }

    ~DnsQueryInfo()
    {
        if (dnsQueryResult.pQueryRecords)
            DnsRecordListFree(dnsQueryResult.pQueryRecords, DnsFreeRecordList);

        if (timer.isValid()) {
            timer_win::cancelTimer(timer);
        }
    }
};

// all active requests and the mutex to access them
struct ActiveDnsRequests
{
    QMutex mutex;
    QHash<unsigned char *, DnsQueryInfo *> queries;
};

ActiveDnsRequests *g_activeDnsRequests = nullptr;   // it's global for access from callbacks

// wrapper for DNS_ADDR_ARRAY
class DnsAddrArray {
public:
    DnsAddrArray(const QStringList &ips)
    {
        int cnt = ips.count();
        if (cnt > 0) {
            p_ = new unsigned char[sizeof(DNS_ADDR_ARRAY) + (cnt - 1) * sizeof(DNS_ADDR)];
            ZeroMemory(p_, sizeof(DNS_ADDR_ARRAY) + (cnt - 1) * sizeof(DNS_ADDR));

            PDNS_ADDR_ARRAY arr = (PDNS_ADDR_ARRAY)p_;
            arr->MaxCount = cnt;
            arr->AddrCount = cnt;

            DWORD error = ERROR_SUCCESS;
            for (int i = 0; i < cnt; ++i) {
                std::wstring strIp = ips[i].toStdWString();
                SOCKADDR_STORAGE sockAddr;
                INT addressLength = sizeof(sockAddr);

                error = WSAStringToAddress((LPWSTR)strIp.c_str(), AF_INET, NULL, (LPSOCKADDR)&sockAddr, &addressLength);
                if (error != ERROR_SUCCESS){
                    addressLength = sizeof(sockAddr);
                    error = WSAStringToAddress((LPWSTR)strIp.c_str(), AF_INET6, NULL, (LPSOCKADDR)&sockAddr, &addressLength);
                }

                if (error != ERROR_SUCCESS) {
                    isValid_ = false;
                    return;
                }
                CopyMemory(arr->AddrArray[i].MaxSa, &sockAddr, DNS_ADDR_MAX_SOCKADDR_LENGTH);
            }
            isValid_ = true;
        } else {
            p_ = nullptr;
            isValid_ = true;
        }
    }

    PDNS_ADDR_ARRAY get()
    {
        WS_ASSERT(isValid_);
        return (PDNS_ADDR_ARRAY)p_;
    }

    bool isValid()
    {
        return isValid_;
    }

    ~DnsAddrArray()
    {
        if (p_)
            delete[] p_;
    }

private:
    bool isValid_;
    unsigned char *p_;
};

void extractResults(DNS_QUERY_RESULT *queryResults, QStringList &outIps, QString &outError)
{
    outIps.clear();
    if (queryResults->QueryStatus == ERROR_SUCCESS) {
        for (PDNS_RECORD p = queryResults->pQueryRecords; p; p = p->pNext) {
            WCHAR ipAddress[128] = {0};

            switch (p->wType) {
            case DNS_TYPE_A:
                IN_ADDR ipv4;
                ipv4.S_un.S_addr = p->Data.A.IpAddress;
                RtlIpv4AddressToStringW(&ipv4, ipAddress);
                break;
            default:
                break;
            }

            outIps << QString::fromStdWString(ipAddress);
        }
    } else if (queryResults->QueryStatus == ERROR_CANCELLED) {
        outError = "DnsQueryEx cancelled by timeout";
    } else {
        outError = "DnsQueryEx failed: " + QString::number(queryResults->QueryStatus);
    }
}


VOID WINAPI queryCompleteCallback(PVOID context, PDNS_QUERY_RESULT queryResults)
{
    if (!g_activeDnsRequests) {
        WS_ASSERT(false);
        return;
    }

    QStringList ips;
    QString errMsg;
    extractResults(queryResults, ips, errMsg);

    unsigned char *requestId = (unsigned char *)context;
    QMutexLocker locker(&g_activeDnsRequests->mutex);
    auto it = g_activeDnsRequests->queries.find(requestId);
    if (it != g_activeDnsRequests->queries.end()) {
        DnsQueryInfo *info = it.value();
        bool bSuccess = QMetaObject::invokeMethod(info->object.get(), "onResolved",
                                              Qt::QueuedConnection, Q_ARG(QStringList, ips), Q_ARG(QString, errMsg), Q_ARG(qint64, info->elapsed.elapsed()));
        WS_ASSERT(bSuccess);
        g_activeDnsRequests->queries.remove(requestId);
        delete info;

    }
}

VOID WINAPI queryCompleteCallbackForBlocked(PVOID context, PDNS_QUERY_RESULT queryResults)
{
    HANDLE *hEvent = (HANDLE *)context;
    SetEvent(*hEvent);
}

void CALLBACK onTimer(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    if (!g_activeDnsRequests) {
        WS_ASSERT(false);
        return;
    }

    unsigned char *requestId = (unsigned char *)lpArgToCompletionRoutine;
    DNS_QUERY_CANCEL dnsQueryCancel;
    bool bFound = false;

    {
        QMutexLocker locker(&g_activeDnsRequests->mutex);
        auto it = g_activeDnsRequests->queries.find(requestId);
        if (it != g_activeDnsRequests->queries.end()) {
            dnsQueryCancel = it.value()->dnsQueryCancel;
            bFound = true;
        }
    }

    if (bFound)
        DnsCancelQuery(&dnsQueryCancel);
}

DnsResolver_win::DnsResolver_win()
{
    WS_ASSERT(g_activeDnsRequests == nullptr);
    activeDnsRequests_ = new ActiveDnsRequests();
    g_activeDnsRequests = activeDnsRequests_;
}

DnsResolver_win::~DnsResolver_win()
{
    std::vector<DNS_QUERY_CANCEL> cancels;
    {
        QMutexLocker locker(&activeDnsRequests_->mutex);
        for (const auto it : qAsConst(activeDnsRequests_->queries)) {
            cancels.push_back(it->dnsQueryCancel);
        }
    }

    for (auto it : cancels)
        DnsCancelQuery(&it);

    qCDebug(LOG_BASIC) << "DnsResolver stopped, active requests = " << activeDnsRequests_->queries.count();

    delete activeDnsRequests_;
    g_activeDnsRequests = nullptr;
}

void DnsResolver_win::lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs)
{
    DnsAddrArray dnsAddrs(dnsServers);
    if (!dnsAddrs.isValid()) {
        QMetaObject::invokeMethod(object.get(), "onResolved",
                                  Qt::QueuedConnection, Q_ARG(QStringList, QStringList()), Q_ARG(QString, "DnsAddrArray failed"), Q_ARG(qint64, 0));
        return;
    }

    QMutexLocker locker(&activeDnsRequests_->mutex);

    DNS_QUERY_REQUEST dnsQueryRequest = {0};
    ZeroMemory(&dnsQueryRequest, sizeof(dnsQueryRequest));

    DnsQueryInfo *info = new DnsQueryInfo();
    info->object = object;
    info->elapsed.start();

    std::wstring domainName = hostname.toStdWString();
    dnsQueryRequest.Version = DNS_QUERY_REQUEST_VERSION1;
    dnsQueryRequest.QueryName = domainName.c_str();
    dnsQueryRequest.QueryType = DNS_TYPE_A;
    dnsQueryRequest.QueryOptions = DNS_QUERY_BYPASS_CACHE;
    dnsQueryRequest.pQueryContext = nextRequestId_;
    dnsQueryRequest.pQueryCompletionCallback = queryCompleteCallback;
    dnsQueryRequest.pDnsServerList = dnsAddrs.get();

    DNS_STATUS err = DnsQueryEx(&dnsQueryRequest, &info->dnsQueryResult, &info->dnsQueryCancel);

    if (err == ERROR_SUCCESS) {     // results are received immediately, without calling callback
        QStringList ips;
        QString errMsg;
        extractResults(&info->dnsQueryResult, ips, errMsg);
        QMetaObject::invokeMethod(object.get(), "onResolved", Qt::QueuedConnection,
                                  Q_ARG(QStringList, ips), Q_ARG(QString, errMsg), Q_ARG(qint64, 0));
        delete info;
        return;
    }
    else if (err != DNS_REQUEST_PENDING) {
        // should never be here
        WS_ASSERT(false);
        // but if suddenly we are here, then we will process it
        QMetaObject::invokeMethod(object.get(), "onResolved", Qt::QueuedConnection,
                                  Q_ARG(QStringList, QStringList()), Q_ARG(QString, "DnsQueryEx failed"), Q_ARG(qint64, 0));
        delete info;
        return;
    }

    activeDnsRequests_->queries[nextRequestId_] = info;
    info->timer.setHandle(timer_win::createTimer(timeoutMs, true, onTimer, nextRequestId_));
    WS_ASSERT(info->timer.isValid());

    nextRequestId_++;
}

QStringList DnsResolver_win::lookupBlocked(const QString &hostname, const QStringList &dnsServers, int timeoutMs, QString *outError)
{
    DnsAddrArray dnsAddrs(dnsServers);
    if (!dnsAddrs.isValid()) {
        if (outError)
            *outError = "DnsAddrArray failed";

        return QStringList();
    }

    wsl::Win32Handle hEvent(CreateEvent(NULL, TRUE, FALSE, NULL));
    WS_ASSERT(hEvent.isValid());

    DNS_QUERY_RESULT dnsQueryResult;
    ZeroMemory(&dnsQueryResult, sizeof(dnsQueryResult));
    dnsQueryResult.Version = DNS_QUERY_RESULTS_VERSION1;

    DNS_QUERY_CANCEL dnsQueryCancel;
    ZeroMemory(&dnsQueryCancel, sizeof(dnsQueryCancel));

    DNS_QUERY_REQUEST dnsQueryRequest = {0};
    ZeroMemory(&dnsQueryRequest, sizeof(dnsQueryRequest));

    std::wstring domainName = hostname.toStdWString();
    dnsQueryRequest.Version = DNS_QUERY_REQUEST_VERSION1;
    dnsQueryRequest.QueryName = domainName.c_str();
    dnsQueryRequest.QueryType = DNS_TYPE_A;
    dnsQueryRequest.QueryOptions = DNS_QUERY_BYPASS_CACHE;
    dnsQueryRequest.pQueryContext = &hEvent;
    dnsQueryRequest.pQueryCompletionCallback = queryCompleteCallbackForBlocked;
    dnsQueryRequest.pDnsServerList = dnsAddrs.get();

    DNS_STATUS err = DnsQueryEx(&dnsQueryRequest, &dnsQueryResult, &dnsQueryCancel);
    if (err != DNS_REQUEST_PENDING && err != ERROR_SUCCESS) {
        // should never be here
        WS_ASSERT(false);
        return QStringList();
    }

    if (err == DNS_REQUEST_PENDING && WaitForSingleObject(hEvent.getHandle(), timeoutMs) == WAIT_TIMEOUT) {
        DnsCancelQuery(&dnsQueryCancel);
        WaitForSingleObject(hEvent.getHandle(), INFINITE);
    }

    QStringList ips;
    QString errMsg;
    extractResults(&dnsQueryResult, ips, errMsg);

    if (outError)
        *outError = errMsg;

    if (dnsQueryResult.pQueryRecords)
        DnsRecordListFree(dnsQueryResult.pQueryRecords, DnsFreeRecordList);

    return ips;
}
