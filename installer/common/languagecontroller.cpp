#include "languagecontroller.h"

#include <QApplication>

#include "utils/languagesutil.h"

LanguageController::LanguageController() : language_(LanguagesUtil::systemLanguage())
{
    loadLanguage(language_);
}

LanguageController::~LanguageController()
{
}

QString LanguageController::getLanguage() const
{
    return language_;
}

bool LanguageController::loadLanguage(const QString &language)
{
    const QString filename = ":/translations/windscribe_installer_" + language + ".qm";

    if (translator_.load(filename)) {
        qApp->installTranslator(&translator_);
        language_ = language;
        return true;
    }

    return false;
}

bool LanguageController::isRtlLanguage() const
{
    if (language_ == "ar" || language_ == "he" || language_ == "fa") {
        return true;
    }
    return false;
}

