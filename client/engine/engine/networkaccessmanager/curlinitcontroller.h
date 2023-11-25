#pragma once

#include <QMutex>

class CurlInitController
{
public:
    CurlInitController();
    ~CurlInitController();

private:
    static bool isInitialized_;
    static QMutex mutex_;
};
