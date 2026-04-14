#pragma once

#include <QObject>

#include <QColor>
#include <QFont>
#include <QString>

class Preferences;

class ThemeController : public QObject
{
    Q_OBJECT
public:
    static ThemeController &instance()
    {
        static ThemeController tc;
        return tc;
    }

    QFont defaultFont(int szPx = 14, int weight = QFont::Normal) const;
    QColor defaultFontColor() const;
    QString defaultFontName() const;

    QColor primaryButtonColor() const;
    QColor primaryButtonHoverColor() const;
    QColor primaryButtonFontColor() const;
    QColor secondaryButtonColor() const;
    QColor secondaryButtonHoverColor() const;

    QColor windowBackgroundColor() const;

private:
    ThemeController();
    ~ThemeController();
};
