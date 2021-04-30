#ifndef TESTCURLNETWORKMANAGER_H
#define TESTCURLNETWORKMANAGER_H

#include <QObject>

class TestCurlNetworkManager : public QObject
{
    Q_OBJECT

public:
    TestCurlNetworkManager();
    ~TestCurlNetworkManager();

private slots:
    void test_get();
    void test_incorrect_get();
    void test_timeout_get();
    void test_ssl_errors();
    void test_iplist_get();

    void test_post();


    void test_async();

private:
};


#endif // TESTCURLNETWORKMANAGER_H
