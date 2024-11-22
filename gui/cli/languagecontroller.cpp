#include "languagecontroller.h"

#include <QCoreApplication>

#include "utils/languagesutil.h"
#include "utils/log/categories.h"

LanguageController::LanguageController()
{
}

LanguageController::~LanguageController()
{
}

QString LanguageController::getLanguage() const
{
    return language_;
}

void LanguageController::setLanguage(const QString &language)
{
    if (language != language_) {
        bool languageLoaded = false;

        // We don't have a language file for English, since all the hard-coded strings are
        // already in English.
        if (language != "en") {
            // Try to load the specified language.  If that fails, try to load the system language.
            languageLoaded = loadLanguage(language);
            if (!languageLoaded) {
                languageLoaded = loadLanguage(LanguagesUtil::systemLanguage());
            }
        }

        if (!languageLoaded) {
            qApp->removeTranslator(&translator_);
            language_ = "en";
        }

        emit languageChanged();
    }
}

bool LanguageController::loadLanguage(const QString &language)
{
    const QString filename = ":/translations/windscribe_cli_" + language + ".qm";

    if (translator_.load(filename)) {
        qCDebug(LOG_CLI) << "LanguageController::setLanguage - language changed:" << language;
        qApp->installTranslator(&translator_);
        language_ = language;
        return true;
    }

    qCDebug(LOG_CLI) << "LanguageController::setLanguage - failed to load language file:" << filename;
    return false;
}
