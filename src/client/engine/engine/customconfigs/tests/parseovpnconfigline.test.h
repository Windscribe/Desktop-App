#pragma once

#include <QObject>

class TestParseOvpnConfigLine : public QObject
{
    Q_OBJECT

private slots:
    void testCommentedRouteControlDirectivesIgnored();
    void testRouteControlDirectivesRequireFirstTokenMatch();
    void testCommentedBlockOutsideDnsIgnored();
    void testBlockOutsideDnsRequiresFirstTokenMatch();
    void testPullFilterStillMatchesWhenCommentContainsEarlierDirectiveName();
};
