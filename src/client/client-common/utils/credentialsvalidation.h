#pragma once

#include <QString>

class CredentialsValidation
{
public:
    enum class UsernameProblem { kNone, kTooShort, kTooLong, kEmailAddress, kDisallowedCharacter };
    enum class PasswordProblem { kNone, kTooShort, kTooLong };

    static UsernameProblem checkUsername(const QString &username);
    static PasswordProblem checkPassword(const QString &password);
};
