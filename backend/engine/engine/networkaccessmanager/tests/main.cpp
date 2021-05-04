#include <QtTest>
#include <QCoreApplication>

#include "tst_dnsrequest.h"
#include "tst_curlnetworkmanager.h"
#include "tst_networkaccessmanager.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    int status = 0;
    //status |= QTest::qExec(new TestDnsRequest() , argc, argv);
    //status |= QTest::qExec(new TestCurlNetworkManager() , argc, argv);
    status |= QTest::qExec(new TestNetworkAccessManager() , argc, argv);

    return status;
}
