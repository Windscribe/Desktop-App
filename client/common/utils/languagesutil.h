#pragma once

#include <QString>

class LanguagesUtil
{
public:
    static QString convertCodeToNative(const QString &code);

    // Returns the two-letter ISO639 language code currently being used by the OS, or "en" otherwise.
    static QString systemLanguage();
};
