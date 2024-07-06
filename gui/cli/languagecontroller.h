#pragma once

#include <QObject>
#include <QTranslator>


class LanguageController : public QObject
{
    Q_OBJECT
public:

    static LanguageController &instance()
    {
        static LanguageController lc;
        return lc;
    }

    void setLanguage(const QString &language);
    QString getLanguage() const;

signals:
    void languageChanged();

private:
    LanguageController();
    ~LanguageController();

    QString language_;

    QTranslator translator_;

    bool loadLanguage(const QString &language);
};
