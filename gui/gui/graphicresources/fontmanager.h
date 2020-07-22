#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <QMap>
#include <QSharedPointer>
#include <QColor>
#include "fontdescr.h"

// manages fonts, use different fonts, depending current language
class FontManager
{
public:
    static FontManager &instance()
    {
        static FontManager fm;
        return fm;
    }

    QFont *getFont(int size, bool isBold, int stretch = 100, qreal letterSpacing = 0.0);
    QFont *getFont(const FontDescr &fd);
    QString getFontStyleSheet(int size, bool isBold);
    void clearCache();

    void languageChanged();

    QColor getCarbonBlackColor();
    QColor getCharcoalColor();
    QColor getMidnightColor();
    QColor getBrightBlueColor();
    QColor getErrorRedColor();
    QColor getBrightYellowColor();

    QString getMidnightColorSS(bool withColorText = true);
    QString getErrorRedColorSS(bool withColorText = true);
    QString getCharcoalColorSS(bool withColorText = true);

    QColor getSeaGreenColor();

private:
    FontManager();
    ~FontManager();

    QMap<QString, QFont *> fonts_;

    void clearFontMap();
};

#endif // FONTMANAGER_H
