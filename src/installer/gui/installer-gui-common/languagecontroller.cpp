#include "languagecontroller.h"

#include <QApplication>
#include <QSettings>
#include <spdlog/spdlog.h>

#include "utils/languagesutil.h"

LanguageController::LanguageController() : language_(LanguagesUtil::systemLanguage())
{
    spdlog::info("System language: {}", language_.toStdString());

    // Attempt to read the language setting from the Windscribe settings file
    QSettings settings("Windscribe", "Windscribe2");
    const QString lang = settings.value("language", language_).toString();

    spdlog::info("Using language: {}", lang.toStdString());
    loadLanguage(lang);
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

