#ifndef TESTPINGNODECONTROLLER_H
#define TESTPINGNODECONTROLLER_H

#include "pinglog.h"
#include "../connectstatecontroller/iconnectstatecontroller.h"

class FakeStateController : public IConnectStateController
{
    Q_OBJECT
public:
    FakeStateController() : IConnectStateController(NULL) {}
    CONNECT_STATE currentState() override;
    CONNECT_STATE prevState() override;

};

// for testing PingNodeController class
class TestPingNodeController
{
public:
    static void doTest();
};

#endif // TESTPINGNODECONTROLLER_H
