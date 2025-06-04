#pragma once

#include <QHash>
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

    QFont getFontWithCustomScale(double scale, double size, int weight, int stretch = 100, qreal letterSpacing = 0.0);
    QFont getFont(double size, int weight, int stretch = 100, qreal letterSpacing = 0.0);
    QFont getFont(const FontDescr &fd);
    QString getFontStyleSheet(double size, int weight);
    void clearCache();

    void languageChanged();

    QColor getLocationsFooterColor();
    QColor getScrollBarBackgroundColor(); // more generic name for this color?
    QColor getCarbonBlackColor();
    QColor getCharcoalColor();
    QColor getMidnightColor();
    QColor getDarkBlueColor();
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
};
