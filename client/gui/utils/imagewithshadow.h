#pragma once

#include <QPainter>

// Class for drawing IndependentPixmap with a shadow (another independent Pixmap).
class ImageWithShadow
{
public:
    explicit ImageWithShadow(const QString &originalName, const QString &shadowName);
    void draw(QPainter *painter, int x, int y);
    int width() const;
    int height() const;
    void updatePixmap();

private:
    QString originalName_;
    QString shadowName_;
    QPixmap pixmap_;
};
