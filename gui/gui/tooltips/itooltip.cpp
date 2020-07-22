#include "itooltip.h"

#include "dpiscalemanager.h"

const int ITooltip::getWidth()
{
    return width_;
}

const int ITooltip::getHeight()
{
    return height_;
}

TooltipShowState ITooltip::getShowState()
{
    return showState_;
}

void ITooltip::setShowState(TooltipShowState showState)
{
    showState_ = showState;
}

const int ITooltip::distanceFromOriginToTailTip()
{
    int distance = 0;
    if (tailType_ == TOOLTIP_TAIL_LEFT)
    {
        int max = leftTooltipMaxY();
        int min = leftTooltipMinY();
        int range = max - min;
        distance = static_cast<int>(tailPosPercent_ * range) + (TOOLTIP_TRIANGLE_WIDTH/2 + TOOLTIP_OFFSET_ROUNDED_CORNER)*G_SCALE;

    }
    else if (tailType_ == TOOLTIP_TAIL_BOTTOM)
    {
        int max = bottomTooltipMaxX();
        int min = bottomTooltipMinX();
        int range = max - min;
        distance = static_cast<int>(tailPosPercent_ * range) + (TOOLTIP_TRIANGLE_HEIGHT + TOOLTIP_OFFSET_ROUNDED_CORNER)*G_SCALE;
    }
    return distance;
}

const int ITooltip::additionalTailWidth()
{
    int tailWidth = 0;
    if (tailType_ == TOOLTIP_TAIL_LEFT) tailWidth = TOOLTIP_TRIANGLE_WIDTH/2 * G_SCALE;
    return tailWidth;
}

const int ITooltip::additionalTailHeight()
{
    int tailHeight = 0;
    if (tailType_ == TOOLTIP_TAIL_BOTTOM) tailHeight = TOOLTIP_TRIANGLE_HEIGHT * G_SCALE;
    return tailHeight;
}

QPolygonF ITooltip::trianglePolygonLeft(int tailLeftPtX, int tailLeftPtY)
{
    // draws counterclockwise from left pt
    QPainterPath path;
    path.moveTo(tailLeftPtX, tailLeftPtY); // left point
    path.lineTo(tailLeftPtX + additionalTailWidth(), tailLeftPtY + TOOLTIP_TRIANGLE_HEIGHT*G_SCALE);
    path.lineTo(tailLeftPtX + additionalTailWidth(), tailLeftPtY - TOOLTIP_TRIANGLE_HEIGHT*G_SCALE);
    path.closeSubpath();
    return path.toFillPolygon();
}

QPolygonF ITooltip::trianglePolygonBottom(int tailLeftPtX, int tailLeftPtY)
{
    // draws counterclockwise from left pt
    QPainterPath path;
    path.moveTo(tailLeftPtX, tailLeftPtY); // left point
    path.lineTo(tailLeftPtX + TOOLTIP_TRIANGLE_WIDTH/2*G_SCALE, tailLeftPtY + TOOLTIP_TRIANGLE_HEIGHT*G_SCALE);
    path.lineTo(tailLeftPtX + TOOLTIP_TRIANGLE_WIDTH*G_SCALE,   tailLeftPtY);
    path.closeSubpath();
    return path.toFillPolygon();
}

void ITooltip::triangleLeftPointXY(int &tailLeftPtX, int &tailLeftPtY)
{
    if (tailType_ == TOOLTIP_TAIL_LEFT)
    {
        int max = leftTooltipMaxY();
        int min = leftTooltipMinY();
        int range = max - min;
        int pos = static_cast<int>(min + tailPosPercent_ * range);

        tailLeftPtX = 0;
        tailLeftPtY = pos;
    }
    else if (tailType_ == TOOLTIP_TAIL_BOTTOM)
    {
        int max = bottomTooltipMaxX();
        int min = bottomTooltipMinX();
        int range = max - min;
        int pos = static_cast<int>(min + tailPosPercent_ * range);

        tailLeftPtX = pos;
        tailLeftPtY = height_ - additionalTailHeight();
    }
}

void ITooltip::initWindowFlags()
{
#ifdef Q_OS_WIN
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#else
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
#endif
    setAttribute(Qt::WA_TranslucentBackground, true);
}

const int ITooltip::leftTooltipMinY()
{
    return TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE + TOOLTIP_TRIANGLE_HEIGHT*G_SCALE;
}

const int ITooltip::leftTooltipMaxY()
{
    return height_ - TOOLTIP_TRIANGLE_HEIGHT*G_SCALE - TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE;
}

const int ITooltip::bottomTooltipMaxX()
{
    return width_ - TOOLTIP_TRIANGLE_WIDTH*G_SCALE - TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE + additionalTailWidth();
}

const int ITooltip::bottomTooltipMinX()
{
    return TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE + additionalTailWidth();
}
