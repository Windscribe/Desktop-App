#include "backgroundpixmapanimation.h"

namespace GuiLocations {

BackgroundPixmapAnimation::BackgroundPixmapAnimation() : isActive_(false)
{

}

void BackgroundPixmapAnimation::startWith(const QPixmap &pixmap, int duration)
{
    pixmap_ = pixmap;
    duration_ = duration;
    timer_.start();
    isActive_ = true;
}

bool BackgroundPixmapAnimation::isAnimationActive()
{
    if (isActive_)
    {
        if (timer_.elapsed() > duration_)
        {
            isActive_ = false;
        }
    }
    return isActive_;
}

double BackgroundPixmapAnimation::curOpacity()
{
    double o = (double)(duration_ - timer_.elapsed()) / (double)duration_;
    if (o < 0) o = 0.0;
    if (o > 1.0) o = 1.0;
    return o;
}

QPixmap &BackgroundPixmapAnimation::curPixmap()
{
    return pixmap_;
}

int BackgroundPixmapAnimation::curX()
{
    double o = (double)(duration_ - timer_.elapsed()) / (double)duration_;
    if (o < 0) o = 0.0;
    if (o > 1.0) o = 1.0;
    return 100 * (1.0-o);
}

} // namespace GuiLocations
