#pragma once

#include <QObject>
#include "engine/networkaccessmanager/curlnetworkmanager.h"

class TestCurlNetworkManager : public QObject
{
    Q_OBJECT

public:
    TestCurlNetworkManager();
    ~TestCurlNetworkManager();

private slots:
    void test_delete_manager();
    void test_get();
    void test_readall_get();
    void test_incorrect_get();

    void test_timeout_get();
    void test_ssl_errors();
    void test_ignore_ssl_error();
    void test_iplist_get();

    void test_post();
    void test_put();
    void test_delete();
    void test_multi();
};


class CurlTestMulti : public QObject
{
    Q_OBJECT

public slots:
    void start();
signals:
    void finished();

private slots:
    void onReplyFinished();
    void removeOne();
    void addMore();

private:
    CurlNetworkManager *manager_;
    QSet<CurlReply *> replies_;
    int finished_;
};


