#include "testpingnodecontroller.h"
#include "pingnodescontroller.h"

void TestPingNodeController::doTest()
{
    /*PingLog pingLog("test_ping.log");
    FakeStateController stateController;
    PingNodesController pnc(NULL, &stateController, pingLog);
    pnc.updateNodes(QStringList() << "1.2.3.4" << "5.6.7.8" << "9.10.11.12");
    pnc.updateNodes(QStringList() << "1.2.3.4" << "9.10.11.12");*/
}

CONNECT_STATE FakeStateController::currentState()
{
    return CONNECT_STATE_DISCONNECTED;
}

CONNECT_STATE FakeStateController::prevState()
{
    return CONNECT_STATE_DISCONNECTED;
}
