#pragma once

#include <QObject>
#include <QTest>

// Unit tests for the deterministic parts of WireGuardConnectionBase: custom-config intake, the
// fetch-answer translation (retcode -> prepare outcome), dual-stack/IPv4-only stripping and endpoint
// assembly. TestWireGuardConnection is a friend of WireGuardConnectionBase, so tests drive the
// answer slot directly instead of going through a live config fetch (which has no seam).
class TestWireGuardConnection : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testCapabilities();
    void testCustomConfigPrepareKeepsDualStack();
    void testCustomConfigPrepareStripsIpv6WhenIpv4Only();
    void testAnswerSuccessDualStack();
    void testAnswerSuccessStripsWithoutNodeIpv6();
    void testAnswerSuccessStripsWhenIpv4Only();
    void testAnswerSuccessInvalidNodeIp();
    void testAnswerKeyLimit();
    void testAnswerFailoverFailed();
    void testAnswerFailed();
};
