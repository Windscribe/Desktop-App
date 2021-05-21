#ifndef BADGEPIXMAP_H
#define BADGEPIXMAP_H

#include <QPainter>

namespace ConnectWindow {

// Class for drawing badge with shadow (used from ConnectStateProtocolPort).
class BadgePixmap
{
public:
    explicit BadgePixmap(const QSize &size, int radius);

    void draw(QPainter *painter, int x, int y);
    void setColor(const QColor &color);
    void setSize(const QSize &size, int radius);

    int width() const;
    int height() const;

private:
    static constexpr int SHADOW_OFFSET = 2;

    QColor shadowColor_;
    QSize size_;
    int radius_;
    QColor badgeBgColor_;
    QPixmap pixmap_;

    void updatePixmap();
};

} //namespace ConnectWindow


#endif // BADGEPIXMAP_H
