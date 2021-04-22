#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <functional>
#include <thread>
#include <mutex>
#include "dnsrequest.h"
#include "areslibraryinit.h"
#include "ares.h"
#include "dnsresolver_test.h"

typedef std::function<void(std::vector<std::string>)> DnsResolverCallback;

// singleton for DNS requests, thread-safe, based on c-ares only
// todo: remove the dependency on Qt (QString, Qt logger)
class DnsResolver
{
public:
    static DnsResolver &instance()
    {
        static DnsResolver s;
        return s;
    }

    DnsResolver(const DnsResolver &) = delete;
    DnsResolver &operator=(const DnsResolver &) = delete;

    // if ips is empty, then use default OS DNS
    // sets the DNS servers for all subsequent requests
    void setDnsServers(const QStringList &ips);

    void lookup(const std::string &hostname, const DnsResolverCallback &callback);
    std::vector<std::string> lookupBlocked(const std::string &hostname);

    // Delete a callback handler and requests for it. A class that has a callback should call this in its destructor.
    void detachCallbackAndRemoveRequests(const DnsResolverCallback &callback);

private:
    explicit DnsResolver();
    virtual ~DnsResolver();

    struct USER_ARG_FOR_BLOCKED
    {
        QStringList ips;
    };


    struct REQUEST_DATA
    {
        std::string hostname;
        ares_channel channel;
        DnsResolverCallback callback;

        // data for options
#ifdef Q_OS_WIN
        QVector<IN_ADDR> dnsServers;
#else
        QVector<in_addr> dnsServers;
#endif

    };

    AresLibraryInit aresLibraryInit_;
    std::thread thread_;
    std::vector< std::shared_ptr<REQUEST_DATA> > activeRequests_;
    std::mutex mutex_;
    std::condition_variable waitCondition_;
    bool bNeedFinish_;
    QStringList dnsServers_;

    static DnsResolver *this_;

    void threadProc();
    QStringList getDnsIps();
    void createOptionsForAresChannel(const QStringList &dnsIps, struct ares_options &options, int &optmask, REQUEST_DATA *requestData);
    static void callbackForNonBlocked(void *arg, int status, int timeouts, struct hostent *host);
    static void callbackForBlocked(void *arg, int status, int timeouts, struct hostent *host);
    // return false, if nothing to process more
    bool processChannel(ares_channel channel);
};

#endif // DNSRESOLVER_H
