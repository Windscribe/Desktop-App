#include "hydraprofile.test.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtTest>

#include "utils/hydraprofile.h"

namespace {

// What `hydra up` writes.
const QByteArray kProfile = R"({
  "version": 2,
  "kind": "windscribe.hydra.profile",
  "hydra_id": "lab",
  "created_at": "2026-10-28T19:00:00Z",
  "fronts": [
    {"location": "Toronto, The 6", "datacenter": "yyz-12-wg.example.com", "datacenter_id": 12, "exit_port": 443,
     "entry_ip": "203.0.113.5", "entry_port": 443, "protocol": "wireguard"},
    {"datacenter": "fra-2-wg.example.com", "datacenter_id": 2, "exit_port": 443, "entry_ip": "203.0.113.5",
     "entry_port": 8443}
  ]
})";

// What an older hydra wrote: one server per front.
const QByteArray kVersion1Profile = R"({"version":1,"kind":"windscribe.hydra.profile","fronts":[{
    "exit_hostname":"ca-114.example","exit_ip":"198.51.100.10","exit_port":443,
    "entry_ip":"203.0.113.5","entry_port":443}]})";

// kProfile's first front with one field replaced (or added).
QByteArray withFrontField(const QString &field, const QString &json)
{
    QString front = R"({"location":"Toronto","datacenter":"yyz-12-wg.example.com","datacenter_id":12,
        "exit_port":443,"entry_ip":"203.0.113.5","entry_port":443})";
    const QRegularExpression existing(QString(R"("%1":("[^"]*"|[^,}]*))").arg(field));
    if (front.contains(existing)) {
        front.replace(existing, QString(R"("%1":%2)").arg(field, json));
    } else {
        front.replace("{", QString(R"({"%1":%2,)").arg(field, json));
    }
    return QString(R"({"version":2,"kind":"windscribe.hydra.profile","fronts":[%1]})").arg(front).toUtf8();
}

bool parses(const QByteArray &json, QString *errorOut = nullptr)
{
    QString error;
    const bool ok = HydraProfile::parse(json, error).has_value();
    if (errorOut) {
        *errorOut = error;
    }
    return ok;
}

}  // namespace

void TestHydraProfile::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TestHydraProfile::init()
{
    HydraProfile::remove();
}

void TestHydraProfile::testParsesTheCliProfile()
{
    QString error;
    const auto p = HydraProfile::parse(kProfile, error);
    QVERIFY2(p.has_value(), qPrintable(error));
    QCOMPARE(p->id(), QString("lab"));
    QCOMPARE(p->fronts().size(), 2);
    QCOMPARE(p->fronts()[0].location, QString("Toronto, The 6"));
    QCOMPARE(p->fronts()[0].datacenter, QString("yyz-12-wg.example.com"));
    QCOMPARE(p->fronts()[0].datacenterId, 12);
    QCOMPARE(p->fronts()[0].endpoint(), QString("203.0.113.5:443"));
    QCOMPARE(p->fronts()[1].exitPort, 443u);
    QCOMPARE(p->fronts()[1].location, QString());
}

void TestHydraProfile::testRejectsUnknownFields()
{
    QString error;
    QVERIFY(!parses(withFrontField("PrivateKey", R"("aGVsbG8=")"), &error));
    QVERIFY2(error.contains("PrivateKey"), qPrintable(error));

    QByteArray topLevel = kProfile;
    topLevel.replace(R"("hydra_id": "lab",)", R"("hydra_id": "lab", "PresharedKey": "x",)");
    QVERIFY(!parses(topLevel, &error));
    QVERIFY2(error.contains("PresharedKey"), qPrintable(error));
}

void TestHydraProfile::testRejectsBadAddressesAndPorts()
{
    QVERIFY(parses(withFrontField("entry_port", "65535")));
    QVERIFY(!parses(withFrontField("entry_ip", R"("2001:db8::1")")));
    QVERIFY(!parses(withFrontField("entry_ip", R"("hydra.example")")));
    QVERIFY(!parses(withFrontField("entry_ip", R"("")")));
    QVERIFY(!parses(withFrontField("entry_port", "0")));
    QVERIFY(!parses(withFrontField("entry_port", "65536")));
    QVERIFY(!parses(withFrontField("entry_port", "443.5")));
    QVERIFY(!parses(withFrontField("exit_port", R"("443")")));
    QVERIFY(!parses(withFrontField("exit_port", "0")));
    QVERIFY(!parses(withFrontField("protocol", R"("openvpn")")));
    QVERIFY(parses(withFrontField("protocol", R"("wireguard")")));
}

void TestHydraProfile::testRejectsWrongKindVersionAndShape()
{
    QByteArray json = kProfile;
    QVERIFY(!parses(json.replace("windscribe.hydra.profile", "windscribe.other")));
    json = kProfile;
    QString error;
    QVERIFY(!parses(json.replace(R"("version": 2)", R"("version": 3)"), &error));
    QVERIFY2(error.contains("version 2"), qPrintable(error));
    json = kProfile;
    QVERIFY(!parses(json.replace(R"("version": 2,)", ""), &error));
    QVERIFY(!parses(R"({"version":2,"kind":"windscribe.hydra.profile","fronts":[]})"));
    QVERIFY(!parses(R"({"version":2,"kind":"windscribe.hydra.profile"})"));
    QVERIFY(!parses("[Interface]\nPrivateKey = x\n"));
    QVERIFY(!parses(QByteArray(HydraProfile::kMaxFileSize + 1, ' ')));
}

void TestHydraProfile::testRefusesAVersion1Profile()
{
    QString error;
    QVERIFY(!parses(kVersion1Profile, &error));
    QCOMPARE(error, QString("This hydra profile is from an older hydra and names one server; ask for a new one."));
}

void TestHydraProfile::testRejectsAServerAddress()
{
    // A version 1 field in a version 2 profile: a front never names one of the datacenter's servers.
    QString error;
    QVERIFY(!parses(withFrontField("exit_ip", R"("198.51.100.10")"), &error));
    QVERIFY2(error.contains("exit_ip"), qPrintable(error));
    QVERIFY(!parses(withFrontField("exit_hostname", R"("ca-114.example")"), &error));
    QVERIFY2(error.contains("exit_hostname"), qPrintable(error));
}

void TestHydraProfile::testDatacenterMustBeAHostname()
{
    QVERIFY(HydraProfile::isHostname("yyz-12-wg.windscribe.com"));
    QVERIFY(HydraProfile::isHostname("locl-1-wg.dev.test"));
    QVERIFY(HydraProfile::isHostname(QString(63, 'a') + ".example"));
    QVERIFY(!HydraProfile::isHostname(QString(64, 'a') + ".example"));
    const QString longest = QString(63, 'a') + "." + QString(63, 'b') + "." + QString(63, 'c') + "." + QString(61, 'd');
    QCOMPARE(longest.size(), 253);
    QVERIFY(HydraProfile::isHostname(longest));
    QVERIFY(!HydraProfile::isHostname(longest + "d"));
    const QStringList bad = {"", "localhost", "a..b", ".a.example", "a.example.", "-a.example", "a-.example", "a b.example",
                             "a_b.example", "a:443.example", "203.0.113.5:443", QString(QChar(0xe9)) + ".example"};
    for (const QString &name : bad) {
        QVERIFY2(!HydraProfile::isHostname(name), qPrintable(name));
    }

    QString error;
    QVERIFY(!parses(withFrontField("datacenter", R"("localhost")"), &error));
    QVERIFY2(error.contains("datacenter"), qPrintable(error));
    QVERIFY(!parses(withFrontField("datacenter", R"(" yyz-12-wg.example.com")")));
    QVERIFY(!parses(withFrontField("datacenter", "12")));
    QVERIFY(!parses(R"({"version":2,"kind":"windscribe.hydra.profile","fronts":[{
        "datacenter_id":12,"exit_port":443,"entry_ip":"203.0.113.5","entry_port":443}]})", &error));
    QVERIFY2(error.contains("datacenter"), qPrintable(error));
}

void TestHydraProfile::testDatacenterIdIsAPositiveWholeNumber()
{
    QVERIFY(parses(withFrontField("datacenter_id", "2147483647")));
    for (const char *bad : {"0", "-1", "12.5", "2147483648", R"("12")", "null"}) {
        QString error;
        QVERIFY2(!parses(withFrontField("datacenter_id", bad), &error), bad);
        QVERIFY2(error.contains("datacenter_id"), qPrintable(error));
    }
    QString error;
    QVERIFY(!parses(R"({"version":2,"kind":"windscribe.hydra.profile","fronts":[{
        "datacenter":"yyz-12-wg.example.com","exit_port":443,"entry_ip":"203.0.113.5","entry_port":443}]})", &error));
    QVERIFY2(error.contains("datacenter_id"), qPrintable(error));
}

void TestHydraProfile::testRejectsDuplicateEntry()
{
    QByteArray json = kProfile;
    QString error;
    QVERIFY(!parses(json.replace(R"("entry_port": 8443)", R"("entry_port": 443)"), &error));
    QVERIFY2(error.contains("203.0.113.5:443"), qPrintable(error));
}

void TestHydraProfile::testMatchesTheDatacenterById()
{
    QString error;
    const auto p = HydraProfile::parse(kProfile, error);
    QVERIFY(p.has_value());
    QCOMPARE(p->frontForDatacenter(12)->endpoint(), QString("203.0.113.5:443"));
    QCOMPARE(p->frontForDatacenter(2)->endpoint(), QString("203.0.113.5:8443"));
    QVERIFY(p->frontForDatacenter(13) == nullptr);
    // A location with no datacenter (a static IP) has id 0.
    QVERIFY(p->frontForDatacenter(0) == nullptr);
}

void TestHydraProfile::testSummaryNamesFrontsByTheServerList()
{
    QString error;
    const auto p = HydraProfile::parse(kProfile, error);
    QVERIFY(p.has_value());

    // Toronto's datacenter is in the server list, under a name of its own; Frankfurt's isn't. The
    // profile's label, free text, is only shown for the one the app can't name.
    const auto serverList = [](int datacenterId) { return datacenterId == 12 ? QString("Toronto - The 6") : QString(); };
    QCOMPARE(p->summary(serverList), QStringList({"Toronto - The 6 via 203.0.113.5:443",
                                                  "fra-2-wg.example.com via 203.0.113.5:8443 (not in the server list)"}));

    const auto emptyList = [](int) { return QString(); };
    QCOMPARE(p->summary(emptyList), QStringList({"Toronto, The 6 via 203.0.113.5:443 (not in the server list)",
                                                 "fra-2-wg.example.com via 203.0.113.5:8443 (not in the server list)"}));
}

void TestHydraProfile::testSaveLoadRemove()
{
    QVERIFY(!HydraProfile::load().has_value());
    QString error;
    QVERIFY2(HydraProfile::save(kProfile, error).has_value(), qPrintable(error));
    const auto loaded = HydraProfile::load();
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->json(), kProfile);
    QVERIFY(HydraProfile::remove());
    QString loadError;
    QVERIFY(!HydraProfile::load(&loadError).has_value());
    QVERIFY(loadError.isEmpty());
    QVERIFY(HydraProfile::remove());
}

void TestHydraProfile::testSaveRefusesInvalidAndKeepsPrevious()
{
    QString error;
    QVERIFY(HydraProfile::save(kProfile, error).has_value());
    QVERIFY(!HydraProfile::save(withFrontField("PrivateKey", R"("x")"), error).has_value());
    QVERIFY(!HydraProfile::save(kVersion1Profile, error).has_value());
    QCOMPARE(HydraProfile::load()->id(), QString("lab"));

    // A file edited by hand into something invalid is ignored rather than half-applied.
    QFile file(HydraProfile::path());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(withFrontField("entry_ip", R"("2001:db8::1")"));
    file.close();
    QVERIFY(!HydraProfile::load().has_value());
}

void TestHydraProfile::testLoadSaysWhyAStoredVersion1ProfileIsUnused()
{
    // An app updated with a version 1 profile still imported: it fronts nothing, and says why.
    QFile file(HydraProfile::path());
    QDir().mkpath(QFileInfo(file).absolutePath());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(kVersion1Profile);
    file.close();

    QString error;
    QVERIFY(!HydraProfile::load(&error).has_value());
    QVERIFY2(error.contains("older hydra"), qPrintable(error));
}

void TestHydraProfile::testLoadDoesNotValidateAStoredProfileAgain()
{
    // Validated when it was imported: a later app with stricter rules keeps using it, and the file stays.
    const QByteArray stored = withFrontField("tag", R"("lab")");
    QString error;
    QVERIFY(!HydraProfile::parse(stored, error).has_value());
    QFile file(HydraProfile::path());
    QDir().mkpath(QFileInfo(file).absolutePath());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(stored);
    file.close();

    const auto loaded = HydraProfile::load();
    QVERIFY(loaded.has_value());
    QVERIFY(loaded->frontForDatacenter(12) != nullptr);
    QVERIFY(QFile::exists(HydraProfile::path()));
}

void TestHydraProfile::testRedactsTheEntryForLogs()
{
    QString error;
    const auto profile = HydraProfile::parse(kProfile, error);
    QVERIFY2(profile.has_value(), qPrintable(error));
    const HydraProfile::Front &front = profile->fronts().first();
    QVERIFY(!front.redactedEndpoint().contains(front.entryIp));
    QCOMPARE(front.redactedEndpoint(), HydraProfile::redactIp(front.entryIp) + ":" + QString::number(front.entryPort));
    QCOMPARE(HydraProfile::redactIp("203.0.113.5"), QString("203.0.113.***"));
}

void TestHydraProfile::testAllIsAnOptionalBoolean()
{
    const auto withAll = [](const QString &value) {
        return QString(R"({"version":2,"kind":"windscribe.hydra.profile","all":%1,"fronts":[{"datacenter":"yyz-12-wg.example.com",
            "datacenter_id":12,"exit_port":443,"entry_ip":"203.0.113.5","entry_port":443}]})").arg(value).toUtf8();
    };
    QString error;
    QVERIFY(HydraProfile::parse(withAll("true"), error)->all());
    QVERIFY(!HydraProfile::parse(withAll("false"), error)->all());
    QVERIFY(!HydraProfile::parse(kProfile, error)->all());
    for (const QString &bad : {QString(R"("true")"), QString("1"), QString("null"), QString("[]")}) {
        QVERIFY2(!parses(withAll(bad), &error), qPrintable(bad));
        QVERIFY2(error.contains("all must be true or false"), qPrintable(error));
    }
    // Still the only new field: anything else is refused, at the top or in a front.
    QVERIFY(!parses(QByteArray(withAll("true")).replace("\"all\"", "\"All\""), &error));
    QVERIFY(!parses(withFrontField("all", "true"), &error));
}

void TestHydraProfile::testLocationsForPreferences()
{
    const auto profileOf = [](int fronts, bool all = false) {
        QStringList list;
        for (int i = 1; i <= fronts; ++i) {
            list << QString(R"({"datacenter":"dc-%1-wg.example.com","datacenter_id":%1,"exit_port":443,
                "entry_ip":"203.0.113.%1","entry_port":443})").arg(i);
        }
        QString error;
        return *HydraProfile::parse(QString(R"({"version":2,"kind":"windscribe.hydra.profile",%1"fronts":[%2]})")
                                        .arg(all ? R"("all":true,)" : "", list.join(","))
                                        .toUtf8(),
                                    error);
    };
    const auto names = [](int datacenterId) { return datacenterId == 4 ? QString() : QString("City %1").arg(datacenterId); };

    auto l = profileOf(1).locations(names);
    QCOMPARE(l.shown, QStringList({"City 1"}));
    QCOMPARE(l.more, 0);
    QVERIFY(!l.all);

    l = profileOf(3).locations(names);
    QCOMPARE(l.shown, QStringList({"City 1", "City 2", "City 3"}));
    QCOMPARE(l.more, 0);

    l = profileOf(5).locations(names);
    QCOMPARE(l.shown, QStringList({"City 1", "City 2", "City 3"}));
    QCOMPARE(l.more, 2);

    l = profileOf(5, true).locations(names);
    QVERIFY(l.all);
    QVERIFY(l.shown.isEmpty());
    QCOMPARE(l.more, 0);

    // A front the server list doesn't have keeps its slot, by the profile's label or hostname.
    QString error;
    const auto p = HydraProfile::parse(kProfile, error);
    l = p->locations([](int datacenterId) { return datacenterId == 12 ? QString("Toronto - The 6") : QString(); });
    QCOMPARE(l.shown, QStringList({"Toronto - The 6", "fra-2-wg.example.com (not in the server list)"}));
}

QTEST_MAIN(TestHydraProfile)
