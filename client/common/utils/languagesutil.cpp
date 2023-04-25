#include "languagesutil.h"

#include <QLocale>

QString LanguagesUtil::convertCodeToNative(const QString &code)
{
    if (code == "en")
        return "English";
    else if (code == "en_nsfw")
        return "English (NSFW)";
    else if (code == "ru")
        return "Русский";
    else if (code == "ar")
        return "العربية";
    else if (code == "es")
        return "Español";
    else if (code == "fr")
        return "Français";
    else if (code == "hu")
        return "Magyar";
    else if (code == "it")
        return "Italiano";
    else if (code == "ja")
        return "日本語";
    else if (code == "ko")
        return "한국어";
    else if (code == "nl")
        return "Nederlands";
    else if (code == "zh")
        return "简体中文";
    else if (code == "de")
        return "Deutsch";
    else if (code == "pl")
        return "Polski";
    else if (code == "tr")
        return "Türkçe";
    else if (code == "cs")
        return "Čeština";
    else if (code == "da")
        return "Dansk";
    else if (code == "el")
        return "Ελληνικά";
    else if (code == "pt")
        return "Português";
    else if (code == "sk")
        return "Slovenčina";
    else if (code == "th")
        return "ไทย";
    else if (code == "vi")
        return "Tiếng Việt";
    else if (code == "sv")
        return "Svenska";
    else if (code == "id")
        return "Indonesia";
    else if (code == "hi")
        return "हिन्दी";
    else if (code == "hr")
        return "Hrvatski jezik";

    return "Unknown";
}

QString LanguagesUtil::systemLanguage()
{
    QString language = QLocale::system().bcp47Name().left(2);
    if (language.isEmpty()) {
        language = "en";
    }

    return language;
}
