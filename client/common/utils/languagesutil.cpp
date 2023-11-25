#include "languagesutil.h"

#include <QLocale>

QString LanguagesUtil::convertCodeToNative(const QString &code)
{
    if (code == "en")
        return "English";
    else if (code == "ar") // Arabic
        return "العربية";
    else if (code == "cs") // Czech
        return "Čeština";
    else if (code == "de") // German
        return "Deutsch";
    else if (code == "es") // Spanish
        return "Español";
    else if (code == "fa") // Farsi
        return "فارسی";
    else if (code == "fr") // French
        return "Français";
    else if (code == "hi") // Hindi
        return "हिन्दी";
    else if (code == "id") // Indonesian
        return "Bahasa Indonesia";
    else if (code == "it") // Italian
        return "Italiano";
    else if (code == "ja") // Japanese
        return "日本語";
    else if (code == "ko") // Korean
        return "한국어";
    else if (code == "pl") // Polish
        return "Polski";
    else if (code == "pt") // Portuguese
        return "Português";
    else if (code == "ru") // Russian
        return "Русский";
    else if (code == "tr") // Turkish
        return "Türkçe";
    else if (code == "uk") // Ukranian
        return "Українська";
    else if (code == "vi") // Vietnamese
        return "Tiếng Việt";
    else if (code == "zh-CN") // Chinese (Simplified)
        return "中文 (简体)";
    else if (code == "zh-TW") // Chinese (Traditional)
        return "中文 (繁體)";

    return "Unknown";
}

QString LanguagesUtil::systemLanguage()
{
    QStringList languages = QLocale::system().uiLanguages();
    QString ret = "en";

    for (QString language : languages) {
        if (language.isEmpty()) {
            continue;
        }
        if (convertCodeToNative(language) == "Unknown") {
            // try with just 2 letters
            language = language.left(2);
            if (convertCodeToNative(language) == "Unknown") {
                continue;
            }
        }
        ret = language;
        break;
    }
    return ret;
}
