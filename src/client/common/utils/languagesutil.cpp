#include "languagesutil.h"

#include <QLocale>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif


QString LanguagesUtil::convertCodeToNative(const QString &code)
{
    if (code == "en")
        return "English";
    else if (code == "ar") // Arabic
        return "العربية";
    else if (code == "be") // Belarusian
        return "беларуская";
    else if (code == "cs") // Czech
        return "Čeština";
    else if (code == "de") // German
        return "Deutsch";
    else if (code == "el") // Greek
        return "Ελληνικά";
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
    else if (code == "sk") // Slovak
        return "Slovenčina";
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

bool LanguagesUtil::isSupportedLanguage(const QString &lang)
{
    static QStringList sl = { "en", "ar", "be", "cs", "de", "el", "es", "fa", "fr", "hi", "id", "it", "ja", "ko", "pl", "pt", "ru", "sk", "tr", "uk", "vi", "zh-CN", "zh-TW" };
    return sl.contains(lang);
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

bool LanguagesUtil::isCensorshipCountry()
{
    // Design Note:
    // Using our systemLanguage() implementation, rather than QLocale::name()/bcp47Name(),
    // since we discovered during development of this class that they do not give the correct
    // language on macOS.
    const QStringList censoredCountries = { "be", "fa", "ru", "tr", "zh" };
    if (censoredCountries.contains(systemLanguage())) {
        return true;
    }

#if defined(Q_OS_WIN)
    // Dynamically load the function to allow the app to run on Windows 10 versions lacking the function export.
    HMODULE hDLL = ::GetModuleHandleA("kernel32.dll");
    if (hDLL != NULL) {
        typedef int (WINAPI* GetUserDefaultGeoNameFunc)(LPWSTR geoName, int geoNameCount);
        GetUserDefaultGeoNameFunc getUserDefaultGeoNameFunc = (GetUserDefaultGeoNameFunc)::GetProcAddress(hDLL, "GetUserDefaultGeoName");
        if (getUserDefaultGeoNameFunc != NULL) {
            // Extra check on Windows using the default geographical location of the user.  This API
            // returns the two-letter ISO 3166-1 code for the default geographical location of the user.
            wchar_t systemGeoName[LOCALE_NAME_MAX_LENGTH];
            int strlen = getUserDefaultGeoNameFunc(systemGeoName, LOCALE_NAME_MAX_LENGTH);
            if (strlen > 0) {
                const QStringList censoredCountriesISO3166 = { "by", "cn", "ir", "ru", "tr" };
                const QString geoName = QString::fromWCharArray(systemGeoName, strlen).toLower();
                if (censoredCountriesISO3166.contains(geoName)) {
                    return true;
                }
            }
        }
    }
#endif

    return false;
}
