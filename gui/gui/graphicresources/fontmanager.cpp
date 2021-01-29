#include "fontmanager.h"
//#include "Gui/options.h"
#include <QApplication>
#include <QThread>
#include <QFont>
#include <QFontDatabase>
#include <QColor>
#include <QFontMetrics>
#include "dpiscalemanager.h"


QFont *FontManager::getFontWithCustomScale(double scale, int size, bool isBold, int stretch, qreal letterSpacing)
{
    Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());

    QString key = QString::number(size);
    if (isBold)
    {
        key += "_1";
    }
    else
    {
        key += "_0";
    }

    key += "_" + QString::number(stretch);
    key += "_" + QString::number(scale, 'f', 2 );
    // qDebug() << "Font key: " << key;

    auto it = fonts_.find(key);
    if (it == fonts_.end())
    {
        QFont *font = new QFont();
        font->setFamily("IBM Plex Sans");
        font->setPixelSize(size * scale);
        font->setBold(isBold);
        font->setStretch(stretch);
        font->setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
        fonts_[key] = font;
        return font;
    }
    else
    {
        return it.value();
    }
}

QFont *FontManager::getFont(int size, bool isBold, int stretch, qreal letterSpacing)
{
    return getFontWithCustomScale(G_SCALE, size, isBold, stretch, letterSpacing);
}

QFont *FontManager::getFont(const FontDescr &fd)
{
    return getFont(fd.size(), fd.isBold(), fd.stretch(), fd.letterSpacing());
}

QString FontManager::getFontStyleSheet(int size, bool isBold)
{
    QString s = QString("font-family: IBM Plex Sans;font-size: %1px").arg(size*G_SCALE);

    if (isBold)
    {
        s += ";font-weight: bold";
    }

    return s;
}

void FontManager::clearCache()
{
    fonts_.clear();
}

void FontManager::languageChanged()
{
    clearFontMap();
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
    return QColor(0x02, 0x0d, 0x1c);
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

    result += "#020d1c";
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
    return QColor(0x55, 0xff, 0x8a);
}

FontManager::FontManager()
{
    QFontDatabase::addApplicationFont(":/Resources/fonts/IBMPlexSans-Bold.ttf");
    QFontDatabase::addApplicationFont(":/Resources/fonts/IBMPlexSans-Regular.ttf");
}

FontManager::~FontManager()
{
    clearFontMap();
}

void FontManager::clearFontMap()
{
    for (auto it = fonts_.begin(); it != fonts_.end(); ++it)
    {
        delete it.value();
    }
    fonts_.clear();
}
