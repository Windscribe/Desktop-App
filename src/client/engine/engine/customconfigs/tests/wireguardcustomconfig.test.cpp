#include "wireguardcustomconfig.test.h"

#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QtTest>
#include <memory>

#include "engine/customconfigs/icustomconfig.h"
#include "engine/customconfigs/wireguardcustomconfig.h"

namespace {

const QString kValidPrivateKey = "aMqdpqAh9LkOLtmCWPSt9CMa/4UjxN3wAXc5KIzU/2A=";
const QString kValidPublicKey = "HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeJ1PKf8ykw=";
const QString kValidPresharedKey = "K6E3jaCAJ5sjQzM6sd1tH8HxX9p2pT8d8gQ7xq8aJ0g=";

// Builds an [Interface] block. Setting a field to QString() (null) omits its
// line; the section itself can be omitted entirely with include=false.
struct Iface {
    QString privateKey = kValidPrivateKey;
    QString address = "10.0.0.2/32";
    QString dns = "1.1.1.1";
    QString extra;
    bool include = true;

    QString build() const
    {
        if (!include) {
            return {};
        }
        QString s = "[Interface]\n";
        if (!privateKey.isNull()) {
            s += QStringLiteral("PrivateKey = %1\n").arg(privateKey);
        }
        if (!address.isNull()) {
            s += QStringLiteral("Address = %1\n").arg(address);
        }
        if (!dns.isNull()) {
            s += QStringLiteral("DNS = %1\n").arg(dns);
        }
        s += extra;
        return s;
    }
};

struct Peer {
    QString publicKey = kValidPublicKey;
    QString presharedKey;
    QString allowedIps = "0.0.0.0/0";
    QString endpoint = "198.51.100.1:51820";
    bool include = true;

    QString build() const
    {
        if (!include) {
            return {};
        }
        QString s = "[Peer]\n";
        if (!publicKey.isNull()) {
            s += QStringLiteral("PublicKey = %1\n").arg(publicKey);
        }
        if (!presharedKey.isNull()) {
            s += QStringLiteral("PresharedKey = %1\n").arg(presharedKey);
        }
        if (!allowedIps.isNull()) {
            s += QStringLiteral("AllowedIPs = %1\n").arg(allowedIps);
        }
        if (!endpoint.isNull()) {
            s += QStringLiteral("Endpoint = %1\n").arg(endpoint);
        }
        return s;
    }
};

std::unique_ptr<customconfigs::ICustomConfig> loadRawConfig(const QString &contents, QString *errOut = nullptr)
{
    QTemporaryFile tmp(QDir::tempPath() + "/wgtest_XXXXXX.conf");
    if (!tmp.open()) {
        return nullptr;
    }
    QTextStream ts(&tmp);
    ts << contents;
    ts.flush();
    const QString path = tmp.fileName();
    tmp.close();
    std::unique_ptr<customconfigs::ICustomConfig> c(customconfigs::WireguardCustomConfig::makeFromFile(path));
    if (errOut && c) {
        *errOut = c->getErrorForIncorrect();
    }
    return c;
}

std::unique_ptr<customconfigs::ICustomConfig> loadConfig(const QString &interfaceExtra, QString *errOut = nullptr)
{
    Iface i;
    i.extra = interfaceExtra;
    return loadRawConfig(i.build() + Peer().build(), errOut);
}

} // namespace

void TestWireguardCustomConfig::testValidWithoutAmneziaParams()
{
    QString err;
    auto c = loadConfig("", &err);
    QVERIFY(c != nullptr);
    QVERIFY2(c->isCorrect(), qPrintable(err));
}

void TestWireguardCustomConfig::testValidHeaderSingleValue()
{
    QString err;
    auto c = loadConfig("H1 = 97329410\n", &err);
    QVERIFY(c != nullptr);
    QVERIFY2(c->isCorrect(), qPrintable(err));
}

void TestWireguardCustomConfig::testValidHeaderRange()
{
    QString err;
    auto c = loadConfig("H1 = 123-456\n", &err);
    QVERIFY(c != nullptr);
    QVERIFY2(c->isCorrect(), qPrintable(err));
}

void TestWireguardCustomConfig::testValidHeaderRangeMinGreaterThanMax()
{
    // AmneziaWG userspace tolerates unordered pairs; the validator must not
    // reject this form (the case that regressed in 2.22.9).
    QString err;
    auto c = loadConfig("H1 = 32894532-8239432\n", &err);
    QVERIFY(c != nullptr);
    QVERIFY2(c->isCorrect(), qPrintable(err));
}

void TestWireguardCustomConfig::testInvalidHeaderHex()
{
    auto c = loadConfig("H1 = 0xABCD\n");
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testInvalidHeaderMultiHyphen()
{
    auto c = loadConfig("H1 = 12-34-56\n");
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testInvalidHeaderTooManyDigits()
{
    // uint32 max is 4294967295 (10 digits); 11 digits must be rejected.
    auto c = loadConfig("H1 = 99999999999\n");
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testMissingInterfaceSection()
{
    Iface i;
    i.include = false;
    auto c = loadRawConfig(i.build() + Peer().build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testMissingPeerSection()
{
    Peer p;
    p.include = false;
    auto c = loadRawConfig(Iface().build() + p.build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testMissingPrivateKey()
{
    Iface i;
    i.privateKey = QString();
    auto c = loadRawConfig(i.build() + Peer().build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testMissingAddress()
{
    Iface i;
    i.address = QString();
    auto c = loadRawConfig(i.build() + Peer().build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testMissingDns()
{
    Iface i;
    i.dns = QString();
    auto c = loadRawConfig(i.build() + Peer().build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testMissingPublicKey()
{
    Peer p;
    p.publicKey = QString();
    auto c = loadRawConfig(Iface().build() + p.build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testInvalidBase64PrivateKey()
{
    Iface i;
    i.privateKey = "not-a-valid-base64-wireguard-key-xxxxxxxxx=";
    auto c = loadRawConfig(i.build() + Peer().build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testInvalidBase64PublicKey()
{
    Peer p;
    p.publicKey = "tooshort=";
    auto c = loadRawConfig(Iface().build() + p.build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testValidPresharedKey()
{
    QString err;
    Peer p;
    p.presharedKey = kValidPresharedKey;
    auto c = loadRawConfig(Iface().build() + p.build(), &err);
    QVERIFY(c != nullptr);
    QVERIFY2(c->isCorrect(), qPrintable(err));
}

void TestWireguardCustomConfig::testInvalidPresharedKey()
{
    Peer p;
    p.presharedKey = "not-a-real-key";
    auto c = loadRawConfig(Iface().build() + p.build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testInvalidAddress()
{
    Iface i;
    i.address = "10.0.0.2/99";  // CIDR out of range for IPv4
    auto c = loadRawConfig(i.build() + Peer().build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testInvalidAllowedIps()
{
    // Octets >255 survive the stripIpv6Address IPv4-prefix filter but fail
    // QHostAddress parsing, exercising the actual AllowedIPs validation.
    Peer p;
    p.allowedIps = "999.999.999.999/32";
    auto c = loadRawConfig(Iface().build() + p.build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testInvalidDns()
{
    Iface i;
    i.dns = "999.999.999.999";
    auto c = loadRawConfig(i.build() + Peer().build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testEndpointPortZero()
{
    Peer p;
    p.endpoint = "198.51.100.1:0";
    auto c = loadRawConfig(Iface().build() + p.build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testEndpointPortTooLarge()
{
    Peer p;
    p.endpoint = "198.51.100.1:99999";
    auto c = loadRawConfig(Iface().build() + p.build());
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testOversizedFile()
{
    // kMaxConfigFileSize = 64 * 1024. Pad with an inert comment section to push
    // the file past that bound while keeping the rest of the config valid.
    QString padding(70 * 1024, QLatin1Char('x'));
    QString contents = Iface().build() + Peer().build() + "\n# " + padding + "\n";
    auto c = loadRawConfig(contents);
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

void TestWireguardCustomConfig::testIValueAtLimit()
{
    // kMaxIValueLength = 4096. A 4096-char hex blob inside <b 0x...> should pass.
    QString hex(4090, QLatin1Char('a'));
    QString iVal = QStringLiteral("<b 0x%1>").arg(hex);  // 4090 + 6 = 4096
    QCOMPARE(iVal.length(), 4096);
    QString err;
    auto c = loadConfig(QStringLiteral("I1 = %1\n").arg(iVal), &err);
    QVERIFY(c != nullptr);
    QVERIFY2(c->isCorrect(), qPrintable(err));
}

void TestWireguardCustomConfig::testIValueTooLong()
{
    QString hex(5000, QLatin1Char('a'));
    QString iVal = QStringLiteral("<b 0x%1>").arg(hex);
    auto c = loadConfig(QStringLiteral("I1 = %1\n").arg(iVal));
    QVERIFY(c != nullptr);
    QVERIFY(!c->isCorrect());
}

QTEST_MAIN(TestWireguardCustomConfig)
