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

QColor WidgetLocationsSizes::getBackgroundColor()
{
    return backgroundColor_;
}

double WidgetLocationsSizes::getScrollingSpeedKef()
{
    return scrollingSpeedKef_ * G_SCALE;
}

WidgetLocationsSizes::WidgetLocationsSizes() : itemHeight_(50), topOffset_(0), scrollBarWidth_(8),
    scrollingSpeedKef_(0.07), backgroundColor_(0x03, 0x09, 0x1C)
{
}

} // namespace GuiLocations
