#pragma once

#include <QPainter>

// Class for drawing text with a shadow. An internal buffer is used, which is redrawn only when the text/font/flags changes.
class TextShadow
{
public:
    explicit TextShadow();
    void drawText(QPainter *painter, const QRect &rect, int flags, const QString &text, QFont &font, const QColor &color);
    // width and height available only after drawText call
    int width() const;
    int height() const;

private:
    QColor shadowColor_;
    QRect lastRect_;
    QRect lastBoundingRect_;
    int lastFlags_;
    QString lastText_;
    QFont lastFont_;
    QColor lastColor_;
    QPixmap pixmap_;

    void updatePixmap();
};
