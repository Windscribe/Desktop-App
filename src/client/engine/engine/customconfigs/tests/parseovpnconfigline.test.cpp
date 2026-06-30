#include <QtTest>

#include "parseovpnconfigline.test.h"

#include "engine/customconfigs/parseovpnconfigline.h"

void TestParseOvpnConfigLine::testCommentedRouteControlDirectivesIgnored()
{
    QCOMPARE(ParseOvpnConfigLine::processLine("# route-nopull only documentation").type,
             ParseOvpnConfigLine::OVPN_CMD_UNKNOWN);
    QCOMPARE(ParseOvpnConfigLine::processLine(" ; route-noexec only documentation").type,
             ParseOvpnConfigLine::OVPN_CMD_UNKNOWN);
}

void TestParseOvpnConfigLine::testRouteControlDirectivesRequireFirstTokenMatch()
{
    QCOMPARE(ParseOvpnConfigLine::processLine("route-nopull").type,
             ParseOvpnConfigLine::OVPN_CMD_IGNORE_REDIRECT_GATEWAY);
    QCOMPARE(ParseOvpnConfigLine::processLine("route-noexec").type,
             ParseOvpnConfigLine::OVPN_CMD_IGNORE_REDIRECT_GATEWAY);
    QCOMPARE(ParseOvpnConfigLine::processLine("setenv NOTE route-nopull").type,
             ParseOvpnConfigLine::OVPN_CMD_UNKNOWN);
    QCOMPARE(ParseOvpnConfigLine::processLine("remote-route-nopull.example 1194 udp").type,
             ParseOvpnConfigLine::OVPN_CMD_UNKNOWN);
}

void TestParseOvpnConfigLine::testCommentedBlockOutsideDnsIgnored()
{
    QCOMPARE(ParseOvpnConfigLine::processLine("# block-outside-dns only documentation").type,
             ParseOvpnConfigLine::OVPN_CMD_UNKNOWN);
    QCOMPARE(ParseOvpnConfigLine::processLine("; block-outside-dns only documentation").type,
             ParseOvpnConfigLine::OVPN_CMD_UNKNOWN);
}

void TestParseOvpnConfigLine::testBlockOutsideDnsRequiresFirstTokenMatch()
{
    QCOMPARE(ParseOvpnConfigLine::processLine("block-outside-dns").type,
             ParseOvpnConfigLine::OVPN_CMD_BLOCK_OUTSIDE_DNS);
    QCOMPARE(ParseOvpnConfigLine::processLine("setenv NOTE block-outside-dns").type,
             ParseOvpnConfigLine::OVPN_CMD_UNKNOWN);
}

void TestParseOvpnConfigLine::testPullFilterStillMatchesWhenCommentContainsEarlierDirectiveName()
{
    QCOMPARE(ParseOvpnConfigLine::processLine("pull-filter ignore redirect-gateway # remote doc").type,
             ParseOvpnConfigLine::OVPN_CMD_IGNORE_REDIRECT_GATEWAY);
}

QTEST_MAIN(TestParseOvpnConfigLine)
