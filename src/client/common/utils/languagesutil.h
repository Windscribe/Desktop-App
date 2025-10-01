#pragma once

#include <QString>

class LanguagesUtil
{
public:
    static QString convertCodeToNative(const QString &code);

    static bool isSupportedLanguage(const QString &lang);

    // Returns the two-letter ISO639 language code currently being used by the OS, or "en" otherwise.
    static QString systemLanguage();

    // Returns true if the system language is set to a country implementing censorship.
    static bool isCensorshipCountry();
};
