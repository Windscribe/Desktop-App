#include "widgetlocationssizes.h"
#include <math.h>
#include "dpiscalemanager.h"

namespace GuiLocations {

int WidgetLocationsSizes::getItemHeight()
{
    return itemHeight_ * G_SCALE;
}

int WidgetLocationsSizes::getTopOffset()
{
    return topOffset_ * G_SCALE;
}

int WidgetLocationsSizes::getScrollBarWidth()
{
    return scrollBarWidth_ * G_SCALE;
}

double WidgetLocationsSizes::getScrollingSpeedKef()
{
    return scrollingSpeedKef_ * G_SCALE;
}

WidgetLocationsSizes::WidgetLocationsSizes() :
    itemHeight_(50),
    topOffset_(0),
    scrollBarWidth_(10),
    scrollingSpeedKef_(0.07)
{
}

} // namespace GuiLocations
