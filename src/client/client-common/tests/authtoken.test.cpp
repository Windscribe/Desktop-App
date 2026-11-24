#include "authtoken.test.h"

#include <QtTest>

#include "api_responses/authtoken.h"

void TestAuthToken::testParse_data()
{
    QTest::addColumn<QString>("json");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isRateLimitError");
    QTest::addColumn<bool>("isBadUsernameError");
    QTest::addColumn<bool>("isUnknownError");
    QTest::addColumn<QString>("token");

    QTest::newRow("token") << QString(R"({"data":{"token":"tok-1"}})") << true << false << false << false << QString("tok-1");
    QTest::newRow("rate limited") << QString(R"({"errorCode":707})") << true << true << false << false << QString();
    QTest::newRow("rejected username") << QString(R"({"errorCode":502,"errorMessage":"1 arguments had validation errors"})")
                                       << true << false << true << false << QString();
    QTest::newRow("unknown error code") << QString(R"({"errorCode":12345})") << true << false << false << true << QString();
    QTest::newRow("unknown error code with data") << QString(R"({"errorCode":12345,"data":{"token":"tok-x"}})")
                                                  << true << false << false << true << QString();
    QTest::newRow("empty token") << QString(R"({"data":{"token":""}})") << false << false << false << false << QString();
    QTest::newRow("no data") << QString(R"({})") << false << false << false << false << QString();
    QTest::newRow("unparsable reply") << QString("<html>blocked</html>") << false << false << false << false << QString();
}

void TestAuthToken::testParse()
{
    QFETCH(QString, json);
    QFETCH(bool, isValid);
    QFETCH(bool, isRateLimitError);
    QFETCH(bool, isBadUsernameError);
    QFETCH(bool, isUnknownError);
    QFETCH(QString, token);

    api_responses::AuthToken authToken(json.toStdString());
    QCOMPARE(authToken.isValid(), isValid);
    QCOMPARE(authToken.isRateLimitError(), isRateLimitError);
    QCOMPARE(authToken.isBadUsernameError(), isBadUsernameError);
    QCOMPARE(authToken.isUnknownError(), isUnknownError);
    QCOMPARE(authToken.token(), token);
}

QTEST_MAIN(TestAuthToken)
