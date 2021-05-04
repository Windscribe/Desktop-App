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

};


#endif // TESTNETWORKACCESSMANAGER_H
