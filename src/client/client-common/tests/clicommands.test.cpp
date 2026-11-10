#include <QtTest>

#include "clicommands.test.h"

#include "ipc/clicommands.h"
#include "utils/hydraprofile.h"

void TestCliCommands::testSetHydraProfileRejectsUnreadableBody()
{
    char empty = 0;
    IPC::CliCommands::SetHydraProfile cmd(&empty, 0);

    QVERIFY(!cmd.isValid_);
}

void TestCliCommands::testSetHydraProfileRoundTrip()
{
    // The CLI reads the file and the service parses it: the bytes cross unchanged, and a version 2 profile
    // still imports on the other side.
    const QByteArray profile = R"({"version":2,"kind":"windscribe.hydra.profile","hydra_id":"lab","fronts":[
        {"location":"Toronto","datacenter":"yyz-12-wg.example.com","datacenter_id":12,"exit_port":443,
         "entry_ip":"203.0.113.5","entry_port":443}]})";
    for (bool remove : {true, false}) {
        IPC::CliCommands::SetHydraProfile sent;
        sent.remove_ = remove;
        sent.json_ = remove ? QByteArray() : profile;
        std::vector<char> data = sent.getData();

        IPC::CliCommands::SetHydraProfile received(data.data(), static_cast<int>(data.size()));

        QVERIFY(received.isValid_);
        QCOMPARE(received.remove_, remove);
        QCOMPARE(received.json_, sent.json_);
        if (!remove) {
            QString error;
            const auto parsed = HydraProfile::parse(received.json_, error);
            QVERIFY2(parsed.has_value(), qPrintable(error));
            QCOMPARE(parsed->fronts()[0].datacenter, QString("yyz-12-wg.example.com"));
        }
    }
}

QTEST_MAIN(TestCliCommands)
