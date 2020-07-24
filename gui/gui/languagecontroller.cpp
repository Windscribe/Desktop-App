#include "languagecontroller.h"

#include <QApplication>
#include "utils/logger.h"

void LanguageController::setLanguage(const QString &language)
{
    if (language != language_)
    {
        if (language == "en")
        {
            qApp->removeTranslator(&translator_);
            qCDebug(LOG_BASIC) << "Language changed:" << language;

            language_ = language;
            emit languageChanged();
        }
        else
        {

#if defined Q_OS_WIN
            QString filename = QApplication::applicationDirPath() + "/languages/" + language + ".qm";
            filename = "C:/work/client-desktop-gui/NewVersion/WindscribeGUI/languages/" + language + ".qm";
#elif defined Q_OS_MAC
            QString filename = QApplication::applicationDirPath() + "/../Languages/" + language + ".qm";
#endif

            if (translator_.load(filename))
            {
                qCDebug(LOG_BASIC) << "Language changed:" << language;
                qApp->installTranslator(&translator_);

                language_ = language;
                emit languageChanged();
            }
            else
            {
                qCDebug(LOG_BASIC) << "Failed load language file for" << language;
                qApp->removeTranslator(&translator_);

                qCDebug(LOG_BASIC) << "Language changed: default language";
            }
        }
    }
}

QString LanguageController::getLanguage() const
{
    return language_;
}

LanguageController::LanguageController()
{
    language_ = "en";
}

LanguageController::~LanguageController()
{

}
