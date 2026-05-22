#pragma once

#include <QObject>
#include <QTest>

class TestOvpnCredentialInliner : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Pass-through cases
    void testEmptyConfig();
    void testNoSensitiveDirectives();
    void testAlreadyInlineBlock();
    void testCommentsPreserved();
    void testDirectiveInsideInlineBlockNotReinlined();

    // Inlining cases
    void testInlineCaRelativePath();
    void testInlineCertAbsolutePath();
    void testInlineDashDashPrefix();
    void testInlineDoubleQuotedPath();
    void testInlineCaseInsensitiveDirective();
    void testInlineTrailingInlineComment();
    void testInlineTlsAuthEmitsKeyDirection();
    void testInlineTlsAuthNoDirection();
    void testInlineTlsCryptV2DropsTrailingFlag();
    void testInlinePkcs12IsBase64Wrapped();

    // Drop cases
    void testBareDirectiveDropped();

    // Reject cases
    void testRejectUnreadableFile();
    void testRejectNulInPath();
    void testRejectOversizedFile();
    void testRejectFileWithAngleBracket();
    void testRejectFileWithLoneClosingBracket();
    void testPkcs12ExemptFromAngleBracketScan();
#ifdef Q_OS_UNIX
    void testRejectFifo();
    void testRejectCharacterDevice();
#endif
};
