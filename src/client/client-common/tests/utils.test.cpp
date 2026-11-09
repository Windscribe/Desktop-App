#include "utils.test.h"

#include <QtTest>

#include "utils/utils.h"

void TestUtils::testRedactCredential_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("one char") << QString("a") << QString("***");
    QTest::newRow("two chars") << QString("ab") << QString("***");
    QTest::newRow("three chars") << QString("abc") << QString("ab***");
    QTest::newRow("server credential") << QString("xq7example-user1") << QString("xq***");
    QTest::newRow("non-ascii") << QString::fromUtf8("\xc3\xbc\xc3\xb1\xc3\xa9x") << QString::fromUtf8("\xc3\xbc\xc3\xb1***");
}

void TestUtils::testRedactCredential()
{
    QFETCH(QString, value);
    QFETCH(QString, expected);

    QCOMPARE(Utils::redactCredential(value), expected);
}

QTEST_MAIN(TestUtils)
