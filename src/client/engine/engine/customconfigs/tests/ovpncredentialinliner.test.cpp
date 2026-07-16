#include <QtTest>

#include "ovpncredentialinliner.test.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "engine/customconfigs/ovpncredentialinliner.h"

using customconfigs::OvpnCredentialInliner;

namespace {

QString writeFile(const QString &dir, const QString &name, const QByteArray &bytes)
{
    QString path = QDir(dir).absoluteFilePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qFatal("Could not open test fixture for writing: %s", qPrintable(path));
    }
    f.write(bytes);
    f.close();
    return path;
}

QByteArray samplePem(const QByteArray &label)
{
    QByteArray out;
    out += "-----BEGIN " + label + "-----\n";
    out += "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvT3aB...sample base64...\n";
    out += "-----END " + label + "-----\n";
    return out;
}

} // namespace

void TestOvpnCredentialInliner::initTestCase()
{
    QCoreApplication::setOrganizationName("WindscribeTest");
    QCoreApplication::setApplicationName("ovpncredentialinliner.test");
}

void TestOvpnCredentialInliner::cleanupTestCase()
{
    QSettings settings;
    settings.clear();
}

void TestOvpnCredentialInliner::testEmptyConfig()
{
    QTemporaryDir dir;
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process("", dir.path());
    QVERIFY(r.success);
    QCOMPARE(r.config, QString(""));
}

void TestOvpnCredentialInliner::testNoSensitiveDirectives()
{
    QTemporaryDir dir;
    QString input =
        "client\n"
        "dev tun\n"
        "proto udp\n"
        "remote 198.51.100.1 1194\n"
        "verb 3\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(r.success);
    QCOMPARE(r.config, input);
}

void TestOvpnCredentialInliner::testAlreadyInlineBlock()
{
    QTemporaryDir dir;
    QString input =
        "client\n"
        "<ca>\n"
        "-----BEGIN CERTIFICATE-----\n"
        "abc\n"
        "-----END CERTIFICATE-----\n"
        "</ca>\n"
        "verb 3\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(r.success);
    QCOMPARE(r.config, input);
}

void TestOvpnCredentialInliner::testCommentsPreserved()
{
    QTemporaryDir dir;
    QString input =
        "# top comment\n"
        "; semi comment\n"
        "client\n"
        "\n"
        "# trailing\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(r.success);
    QCOMPARE(r.config, input);
}

void TestOvpnCredentialInliner::testDirectiveInsideInlineBlockNotReinlined()
{
    QTemporaryDir dir;
    // The literal text "ca foo.crt" appears inside a <cert> block. The inliner
    // must treat it as block content, not as a directive to inline.
    QString input =
        "<cert>\n"
        "ca foo.crt\n"
        "</cert>\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(r.success);
    QCOMPARE(r.config, input);
}

void TestOvpnCredentialInliner::testInlineCaRelativePath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray pem = samplePem("CERTIFICATE");
    writeFile(dir.path(), "ca.crt", pem);

    QString input = "ca ca.crt\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<ca>\n"));
    QVERIFY(r.config.contains("</ca>\n"));
    QVERIFY(r.config.contains(QString::fromUtf8(pem)));
    QVERIFY(!r.config.contains("ca ca.crt"));
}

void TestOvpnCredentialInliner::testInlineCertAbsolutePath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray pem = samplePem("CERTIFICATE");
    QString absPath = writeFile(dir.path(), "client.crt", pem);

    QString input = QString("cert %1\n").arg(absPath);
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<cert>\n"));
    QVERIFY(r.config.contains(QString::fromUtf8(pem)));
}

void TestOvpnCredentialInliner::testInlineDashDashPrefix()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray pem = samplePem("PRIVATE KEY");
    writeFile(dir.path(), "client.key", pem);

    QString input = "--key client.key\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<key>\n"));
    QVERIFY(r.config.contains(QString::fromUtf8(pem)));
}

void TestOvpnCredentialInliner::testInlineDoubleQuotedPath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray pem = samplePem("CERTIFICATE");
    writeFile(dir.path(), "with space.crt", pem);

    QString input = "ca \"with space.crt\"\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<ca>\n"));
    QVERIFY(r.config.contains(QString::fromUtf8(pem)));
}

void TestOvpnCredentialInliner::testInlineCaseInsensitiveDirective()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray pem = samplePem("CERTIFICATE");
    writeFile(dir.path(), "ca.crt", pem);

    QString input = "CA ca.crt\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    // The directive name in the emitted inline tag is lowercase regardless of
    // the input casing.
    QVERIFY(r.config.contains("<ca>\n"));
    QVERIFY(r.config.contains("</ca>\n"));
}

void TestOvpnCredentialInliner::testInlineTrailingInlineComment()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray pem = samplePem("CERTIFICATE");
    writeFile(dir.path(), "ca.crt", pem);

    QString input = "ca ca.crt # this comment must not become a second arg\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<ca>\n"));
}

void TestOvpnCredentialInliner::testInlineTlsAuthEmitsKeyDirection()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray keyMaterial = "-----BEGIN OpenVPN Static key V1-----\nabc\n-----END OpenVPN Static key V1-----\n";
    writeFile(dir.path(), "ta.key", keyMaterial);

    QString input = "tls-auth ta.key 1\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<tls-auth>\n"));
    QVERIFY(r.config.contains("</tls-auth>\n"));
    QVERIFY(r.config.contains("key-direction 1\n"));
}

void TestOvpnCredentialInliner::testInlineTlsAuthNoDirection()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray keyMaterial = "-----BEGIN OpenVPN Static key V1-----\nabc\n-----END OpenVPN Static key V1-----\n";
    writeFile(dir.path(), "ta.key", keyMaterial);

    QString input = "tls-auth ta.key\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<tls-auth>\n"));
    QVERIFY(!r.config.contains("key-direction"));
}

void TestOvpnCredentialInliner::testInlineTlsCryptV2DropsTrailingFlag()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray keyMaterial = "-----BEGIN OpenVPN tls-crypt-v2 client key-----\nabc\n-----END OpenVPN tls-crypt-v2 client key-----\n";
    writeFile(dir.path(), "tcv2.key", keyMaterial);

    QString input = "tls-crypt-v2 tcv2.key force-cookie\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<tls-crypt-v2>\n"));
    QVERIFY(r.config.contains("</tls-crypt-v2>\n"));
    QVERIFY(!r.config.contains("force-cookie"));
}

void TestOvpnCredentialInliner::testInlinePkcs12IsBase64Wrapped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // Binary blob with bytes that aren't valid UTF-8 — would corrupt a naive
    // text inline. Must be base64-encoded.
    QByteArray binary;
    for (int i = 0; i < 256; i++) {
        binary.append(static_cast<char>(i));
    }
    writeFile(dir.path(), "client.p12", binary);

    QString input = "pkcs12 client.p12\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<pkcs12>\n"));
    QVERIFY(r.config.contains("</pkcs12>\n"));

    // Extract block body and confirm it decodes back to the original bytes.
    int openTag = r.config.indexOf("<pkcs12>\n");
    int closeTag = r.config.indexOf("</pkcs12>\n");
    QVERIFY(openTag >= 0);
    QVERIFY(closeTag > openTag);
    QString body = r.config.mid(openTag + QString("<pkcs12>\n").size(),
                                closeTag - openTag - QString("<pkcs12>\n").size());
    // Confirm 64-column wrapping by checking that no body line is longer than 64.
    for (const QString &line : body.split('\n')) {
        QVERIFY2(line.size() <= 64, qPrintable(QString("Base64 line too long: %1 chars").arg(line.size())));
    }
    QByteArray decoded = QByteArray::fromBase64(body.toLatin1());
    QCOMPARE(decoded, binary);
}

void TestOvpnCredentialInliner::testBareDirectiveDropped()
{
    QTemporaryDir dir;
    QString input =
        "client\n"
        "ca\n"
        "verb 3\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(r.success);
    QStringList outputLines = r.config.split('\n');
    QVERIFY(!outputLines.contains("ca"));
    QVERIFY(outputLines.contains("client"));
    QVERIFY(outputLines.contains("verb 3"));
}

void TestOvpnCredentialInliner::testRejectUnreadableFile()
{
    QTemporaryDir dir;
    QString input = "ca does-not-exist.crt\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(!r.success);
    QVERIFY(!r.errorMessage.isEmpty());
    QVERIFY(r.errorMessage.contains("ca"));
}

void TestOvpnCredentialInliner::testRejectNulInPath()
{
    QTemporaryDir dir;
    QString input;
    input += "ca bad";
    input += QChar(QChar::Null);
    input += "path.crt\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(!r.success);
    QVERIFY(r.errorMessage.contains("invalid"));
}

void TestOvpnCredentialInliner::testRejectOversizedFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray big(2 * 1024 * 1024, 'x');
    writeFile(dir.path(), "huge.crt", big);

    QString input = "ca huge.crt\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(!r.success);
    QVERIFY(r.errorMessage.contains("limit") || r.errorMessage.contains("larger"));
}

void TestOvpnCredentialInliner::testRejectFileWithAngleBracket()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // Legitimate PEM never contains '<'. A file with the smuggling pattern
    // (closing our opening tag and opening a different one) is rejected by
    // the angle-bracket rule before the bytes ever reach the helper.
    QByteArray smuggled =
        "-----BEGIN CERTIFICATE-----\n"
        "legitlookingbase64\n"
        "-----END CERTIFICATE-----\n"
        "</ca>\n"
        "<auth-user-pass>\n"
        "planted_user\n"
        "planted_password\n"
        "</auth-user-pass>\n"
        "<ca>\n"
        "more cert content\n";
    writeFile(dir.path(), "smuggled.crt", smuggled);

    QString input = "ca smuggled.crt\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(!r.success);
    QVERIFY2(r.errorMessage.contains("'<' or '>'"), qPrintable(r.errorMessage));
}

void TestOvpnCredentialInliner::testRejectFileWithLoneClosingBracket()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // A single '<' or '>' anywhere in the content is enough — PEM/static-key
    // payloads have no business containing one.
    QByteArray smuggled = "real PEM\n  </key>\nmore stuff\n";
    writeFile(dir.path(), "sneaky.key", smuggled);

    QString input = "key sneaky.key\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(!r.success);
    QVERIFY2(r.errorMessage.contains("'<' or '>'"), qPrintable(r.errorMessage));
}

void TestOvpnCredentialInliner::testPkcs12ExemptFromAngleBracketScan()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // pkcs12 files are arbitrary binary — '<' bytes are entirely plausible
    // and not malicious. Because we base64-encode the bytes, any embedded
    // brackets are opaque by the time the helper parses the config.
    QByteArray binary = "binarydata-prefix\n<ca>\nbinary-suffix\n";
    writeFile(dir.path(), "tricky.p12", binary);

    QString input = "pkcs12 tricky.p12\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY2(r.success, qPrintable(r.errorMessage));
    QVERIFY(r.config.contains("<pkcs12>\n"));
    QVERIFY(!r.config.contains("\n<ca>\n"));
}

#ifdef Q_OS_UNIX
void TestOvpnCredentialInliner::testRejectFifo()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QByteArray fifoPath = QDir(dir.path()).absoluteFilePath("evil.crt").toLocal8Bit();
    QVERIFY(::mkfifo(fifoPath.constData(), 0600) == 0);

    // The inliner must reject the FIFO outright. If it didn't, QFile::read()
    // would block waiting for a writer that will never arrive — this test would
    // hang instead of failing.
    QString input = "ca evil.crt\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(!r.success);
    QVERIFY2(r.errorMessage.contains("regular"), qPrintable(r.errorMessage));
}

void TestOvpnCredentialInliner::testRejectCharacterDevice()
{
    QTemporaryDir dir;
    // /dev/zero reports size 0 but yields unbounded reads. The inliner must
    // reject character devices outright; otherwise this test would OOM or hang.
    QString input = "ca /dev/zero\n";
    OvpnCredentialInliner::Result r = OvpnCredentialInliner::process(input, dir.path());
    QVERIFY(!r.success);
    QVERIFY2(r.errorMessage.contains("regular"), qPrintable(r.errorMessage));
}
#endif

QTEST_MAIN(TestOvpnCredentialInliner)
