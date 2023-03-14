#include "languagecontroller.h"

#include <QApplication>
#include "utils/ws_assert.h"
#include "utils/logger.h"

void LanguageController::setLanguage(const QString &language)
{
    if (language != language_) {
        QString filename = ":/i18n/" + language + ".qm";

        if (translator_.load(filename)) {
            qCDebug(LOG_BASIC) << "Language changed:" << language;
            qApp->installTranslator(&translator_);

            language_ = language;
        } else {
            qCDebug(LOG_BASIC) << "Failed load language file for: " << language;
            qApp->removeTranslator(&translator_);

            qCDebug(LOG_BASIC) << "Language changed: default language";
        }
        emit languageChanged();
    }
}

QString LanguageController::getLanguage() const
{
    return language_;
}

LanguageController::LanguageController() : language_("en")
{
}

LanguageController::~LanguageController()
{

}
