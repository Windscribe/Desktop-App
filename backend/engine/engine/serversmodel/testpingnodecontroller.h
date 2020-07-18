#ifndef TESTPINGNODECONTROLLER_H
#define TESTPINGNODECONTROLLER_H

#include "pinglog.h"
#include "../connectstatecontroller/iconnectstatecontroller.h"

class FakeStateController : public IConnectStateController
{
    Q_OBJECT
public:
    FakeStateController() : IConnectStateController(NULL) {}
    virtual CONNECT_STATE currentState();
    virtual CONNECT_STATE prevState();

};

// for testing PingNodeController class
class TestPingNodeController
{
public:
    static void doTest();
};

#endif // TESTPINGNODECONTROLLER_H
