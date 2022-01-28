#ifndef TESTNETWORKACCESSMANAGER_H
#define TESTNETWORKACCESSMANAGER_H

#include <QObject>

class TestNetworkAccessManager : public QObject
{
    Q_OBJECT

public:
    TestNetworkAccessManager();
    ~TestNetworkAccessManager();

private slots:
    void test_get();
    void test_post();
    void test_put();
    void test_delete();

};


#endif // TESTNETWORKACCESSMANAGER_H
