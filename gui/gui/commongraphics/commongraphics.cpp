#include "commongraphics.h"

#include <QFontMetrics>
#include <qmath.h>
#include "dpiscalemanager.h"

namespace CommonGraphics {

int centeredOffset(int background_length, int graphic_length)
{
    return background_length/2 - graphic_length/2;
}

int textWidth(QString text, const QFont &font)
{
    QFontMetrics fontMetrics(font);
    return fontMetrics.horizontalAdvance(text);
}

int textHeight(const QFont &font)
{
    QFontMetrics fontMetrics(font);
    return fontMetrics.height();
}

int idealWidth(int minWidth, int maxWidth, int idealLines, int oneLineWidth)
{
    int width = minWidth;
    int lines = qCeil((double)oneLineWidth/width);
    while (lines > idealLines && width < maxWidth)
    {
        width += 5;
        lines = qCeil((double)oneLineWidth/width);
    }

    return width;
}

int idealWidthOfTextRect(int minWidth, int maxWidth, int idealLines, QString text, const QFont &font)
{
    QFontMetrics fm(font);
    int oneLineWidth = fm.horizontalAdvance(text);
    oneLineWidth *= 1.1; // Actual text length will be a bit larger than the one-liner text due to centering per line
    return idealWidth(minWidth, maxWidth, idealLines, oneLineWidth);
}


QRect textBoundingRect(QFont font, int posX, int posY, int width, QString message, Qt::TextFlag flag)
{
    QFontMetrics fm(font);
    return fm.boundingRect(QRect(posX, posY, width, 10), flag, message);
}

QRect idealRect(int posX, int posY, int minWidth, int maxWidth, int idealLines, QString text, QFont font, Qt::TextFlag flag)
{
    const int ideal_width = idealWidthOfTextRect(minWidth, maxWidth, idealLines, text, font);
    return textBoundingRect(font, posX, posY, ideal_width, text, flag);
}

QString truncatedText(QString original, QFont font, int width)
{
    QString shortened = original;

    int lastRemovedIndex = -1;
    int count = 0;
    while (CommonGraphics::textWidth(shortened, font) > width)
    {
        lastRemovedIndex = shortened.length()/2;
        shortened.remove(lastRemovedIndex, 1);
        count++;
    }

    if (lastRemovedIndex != -1)
    {
        shortened.insert(lastRemovedIndex, "...");
    }

    if (count <= 3) shortened = original;

    return shortened;
}

QString maybeTruncatedText(const QString &original, const QFont &font, int width)
{
    QString newText = original;
    if (CommonGraphics::textWidth(original, font) > width)
    {
        newText = CommonGraphics::truncatedText(original, font, width);
    }
    return newText;
}

}
