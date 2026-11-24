#include "credentialsvalidation.test.h"

#include <QtTest>

using UsernameProblem = CredentialsValidation::UsernameProblem;
using PasswordProblem = CredentialsValidation::PasswordProblem;

void TestCredentialsValidation::testCheckUsername_data()
{
    QTest::addColumn<QString>("username");
    QTest::addColumn<UsernameProblem>("expected");

    QTest::newRow("plain") << QString("user_name-1") << UsernameProblem::kNone;
    QTest::newRow("email address") << QString("me@example.com") << UsernameProblem::kEmailAddress;
    QTest::newRow("at sign") << QString("user@") << UsernameProblem::kEmailAddress;
    QTest::newRow("at sign wins over character") << QString("me!@example.com") << UsernameProblem::kEmailAddress;
    QTest::newRow("account hash") << QString("0x0123456789abcdef0123456789abcdef") << UsernameProblem::kNone;
    QTest::newRow("shortest") << QString("abc") << UsernameProblem::kNone;
    QTest::newRow("longest") << QString(255, 'a') << UsernameProblem::kNone;
    QTest::newRow("surrounding spaces trimmed") << QString("  abc  ") << UsernameProblem::kNone;
    QTest::newRow("empty") << QString() << UsernameProblem::kTooShort;
    QTest::newRow("too short") << QString("ab") << UsernameProblem::kTooShort;
    QTest::newRow("too short once trimmed") << QString("  ab  ") << UsernameProblem::kTooShort;
    QTest::newRow("too long") << QString(256, 'a') << UsernameProblem::kTooLong;
    QTest::newRow("long but fits once trimmed") << QString(" ") + QString(255, 'a') + QString(" ") << UsernameProblem::kNone;

    const QString disallowed = QStringLiteral("\\|/!&%`~?$#<>.+ ");
    for (const QChar c : disallowed) {
        QTest::addRow("contains '%s'", QString(c).toUtf8().constData()) << QString("us") + c + QString("er")
                                                                        << UsernameProblem::kDisallowedCharacter;
    }
    QTest::newRow("non-ASCII letter") << QString::fromUtf8("josé") << UsernameProblem::kDisallowedCharacter;
    QTest::newRow("full-width digit") << QString::fromUtf8("user１") << UsernameProblem::kDisallowedCharacter;
    QTest::newRow("too long wins over character") << QString(256, '!') << UsernameProblem::kTooLong;
}

void TestCredentialsValidation::testCheckUsername()
{
    QFETCH(QString, username);
    QFETCH(UsernameProblem, expected);
    QCOMPARE(CredentialsValidation::checkUsername(username), expected);
}

void TestCredentialsValidation::testCheckPassword_data()
{
    QTest::addColumn<QString>("password");
    QTest::addColumn<PasswordProblem>("expected");

    QTest::newRow("shortest") << QString("abcdefgh") << PasswordProblem::kNone;
    QTest::newRow("longest") << QString(255, 'a') << PasswordProblem::kNone;
    QTest::newRow("any characters") << QString("p@ss !|/<>") << PasswordProblem::kNone;
    QTest::newRow("spaces count") << QString("        ") << PasswordProblem::kNone;
    QTest::newRow("empty") << QString() << PasswordProblem::kTooShort;
    QTest::newRow("too short") << QString("abcdefg") << PasswordProblem::kTooShort;
    QTest::newRow("too long") << QString(256, 'a') << PasswordProblem::kTooLong;
}

void TestCredentialsValidation::testCheckPassword()
{
    QFETCH(QString, password);
    QFETCH(PasswordProblem, expected);
    QCOMPARE(CredentialsValidation::checkPassword(password), expected);
}

QTEST_MAIN(TestCredentialsValidation)
