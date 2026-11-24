#include "credentialsvalidation.h"

namespace {
const int kMinUsernameLength = 3;
const int kMinPasswordLength = 8;
const int kMaxLength = 255;

bool isAllowedUsernameCharacter(QChar c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}
}

CredentialsValidation::UsernameProblem CredentialsValidation::checkUsername(const QString &username)
{
    const QString trimmed = username.trimmed();
    if (trimmed.length() < kMinUsernameLength) {
        return UsernameProblem::kTooShort;
    }
    if (trimmed.length() > kMaxLength) {
        return UsernameProblem::kTooLong;
    }
    if (trimmed.contains('@')) {
        return UsernameProblem::kEmailAddress;
    }
    for (const QChar c : trimmed) {
        if (!isAllowedUsernameCharacter(c)) {
            return UsernameProblem::kDisallowedCharacter;
        }
    }
    return UsernameProblem::kNone;
}

CredentialsValidation::PasswordProblem CredentialsValidation::checkPassword(const QString &password)
{
    if (password.length() < kMinPasswordLength) {
        return PasswordProblem::kTooShort;
    }
    if (password.length() > kMaxLength) {
        return PasswordProblem::kTooLong;
    }
    return PasswordProblem::kNone;
}
