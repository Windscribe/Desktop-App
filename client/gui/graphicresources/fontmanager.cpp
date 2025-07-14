#include "fontmanager.h"
//#include "Gui/options.h"
#include <QApplication>
#include <QThread>
#include <QFont>
#include <QFontDatabase>
#include <QColor>
#include <QFontMetrics>
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"

QFont FontManager::getFontWithCustomScale(double scale, double size, int weight, int stretch, qreal letterSpacing)
{
    WS_ASSERT(QApplication::instance()->thread() == QThread::currentThread());

    QFont font;
    font.setFamily("IBM Plex Sans");
    font.setPixelSize(size * scale);
    font.setWeight(QFont::Weight(weight));
    font.setStretch(stretch);
    font.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
    return font;
}

QFont FontManager::getFont(double size, int weight, int stretch, qreal letterSpacing)
{
    return getFontWithCustomScale(G_SCALE, size, weight, stretch, letterSpacing);
}

QFont FontManager::getFont(const FontDescr &fd)
{
    return getFont(fd.size(), fd.weight(), fd.stretch(), fd.letterSpacing());
}

QString FontManager::getFontStyleSheet(double size, int weight)
{
    QString s = QString("font-family: IBM Plex Sans;font-size: %1px").arg(size*G_SCALE);

    s += ";font-weight: " + QString::number(weight);
    return s;
}

void FontManager::languageChanged()
{
}

QColor FontManager::getLocationsFooterColor()
{
    return QColor(26, 39, 58);
}

QColor FontManager::getScrollBarBackgroundColor()
{
    return QColor(76, 90, 100);
}

QColor FontManager::getCarbonBlackColor()
{
    return QColor(0x1A, 0x27, 0x3A);
}

QColor FontManager::getCharcoalColor()
{
    return QColor(0x27, 0x31, 0x3d);
}

QColor FontManager::getMidnightColor()
{
    return QColor(0x09, 0x0e, 0x19);
}

QColor FontManager::getDarkBlueColor()
{
    return QColor(0x18, 0x22, 0x2f);
}

QColor FontManager::getBrightBlueColor()
{
    return QColor(0x00, 0x6a, 0xff);
}

QColor FontManager::getErrorRedColor()
{
    return QColor(0xff, 0x3b, 0x3b);
}

QColor FontManager::getBrightYellowColor()
{
    return QColor(0xff, 0xef, 0x02);
}

QString FontManager::getMidnightColorSS(bool withColorText)
{
    QString result = "";
    if (withColorText) result += "color: ";

    result += "#090E19";
    return result;
}

QString FontManager::getErrorRedColorSS(bool withColorText)
{
    QString result = "";
    if (withColorText) result += "color: ";

    result += "#ff3b3b";
    return result;
}

QString FontManager::getCharcoalColorSS(bool withColorText)
{
    QString result = "";
    if (withColorText) result += "color: ";
    result += "#27313d";

    return result;
}

QColor FontManager::getSeaGreenColor()
{
    return QColor(0x17, 0xE9, 0xAD);
}

FontManager::FontManager()
{
    QFontDatabase::addApplicationFont(":/resources/fonts/ibmplexsans.ttf");
}

FontManager::~FontManager()
{
}
