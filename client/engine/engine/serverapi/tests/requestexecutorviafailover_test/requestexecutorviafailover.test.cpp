#include "requestexecutorviafailover.test.h"
#include "engine/serverapi/requests/loginrequest.h"
#include <QtTest>
#include <QtConcurrent/QtConcurrent>

RequestExecuterViaFailover_test::RequestExecuterViaFailover_test()
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

}

QTEST_MAIN(RequestExecuterViaFailover_test)
