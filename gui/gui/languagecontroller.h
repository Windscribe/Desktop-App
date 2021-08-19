#ifndef LANGUAGECONTROLLER_H
#define LANGUAGECONTROLLER_H

#include <QObject>
#include <QTranslator>

// TODO: make persistent currentLanguage

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

};

#endif // LANGUAGECONTROLLER_H
