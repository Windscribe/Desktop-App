#ifndef BACKGROUNDPIXMAPANIMATION_H
#define BACKGROUNDPIXMAPANIMATION_H

#include <QElapsedTimer>
#include <QPixmap>

namespace GuiLocations {

class BackgroundPixmapAnimation
{
public:
    BackgroundPixmapAnimation();
    void startWith(const QPixmap &pixmap, int duration);
    bool isAnimationActive();
    double curOpacity();
    QPixmap &curPixmap();
    int curX();

private:
    QPixmap pixmap_;
    bool isActive_;
    int duration_;
    QElapsedTimer timer_;
};

} // namespace GuiLocations

#endif // BACKGROUNDPIXMAPANIMATION_H
