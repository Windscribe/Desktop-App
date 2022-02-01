#include <QtTest>
#include <QCoreApplication>
//#include <vld.h>

#include "tst_dnsrequest.h"
#include "tst_curlnetworkmanager.h"
#include "tst_networkaccessmanager.h"
#include "tst_dnscache.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    int status = 0;
    //QScopedPointer<TestDnsRequest> d(new TestDnsRequest());
    //status |= QTest::qExec(d.get() , argc, argv);

    //QScopedPointer<TestCurlNetworkManager> d(new TestCurlNetworkManager());
    //status |= QTest::qExec(d.get() , argc, argv);

    status |= QTest::qExec(new TestNetworkAccessManager() , argc, argv);

    //QScopedPointer<TestDnsCache> d(new TestDnsCache());
    //status |= QTest::qExec(d.get() , argc, argv);

    return status;
}
