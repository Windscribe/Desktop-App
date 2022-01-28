#ifndef CURLINITCONTROLLER_H
#define CURLINITCONTROLLER_H

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

#endif // CURLINITCONTROLLER_H
