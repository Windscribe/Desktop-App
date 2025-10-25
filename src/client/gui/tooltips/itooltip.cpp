#include "itooltip.h"
#include <QPainterPath>
#include <QPropertyAnimation>
#include "dpiscalemanager.h"

int ITooltip::getWidth() const
{
    return width_;
}

int ITooltip::getHeight() const
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

bool ITooltip::getAnimate() const
{
    return animate_;
}

int ITooltip::getAnimationSpeed() const
{
    return animationSpeed_;
}

int ITooltip::distanceFromOriginToTailTip() const
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

int ITooltip::additionalTailWidth() const
{
    int tailWidth = 0;
    if (tailType_ == TOOLTIP_TAIL_LEFT) tailWidth = TOOLTIP_TRIANGLE_WIDTH/2 * G_SCALE;
    return tailWidth;
}

int ITooltip::additionalTailHeight() const
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
#if defined(Q_OS_WIN)
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#elif defined(Q_OS_LINUX)
    // In Wayland, setting this to any flag which implies Qt::Window causes the tooltip to be misplaced,
    // and causes side effects such as window clipping, since window position can not controlled by
    // the application, and top-level windows without parents behave strangely.
    // Setting just Qt::ToolTip seems to look and behave fine on both Xorg and Wayland.
    setWindowFlags(Qt::ToolTip);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
#else
    // Removed Qt::Tooltip since inner Qt::Popup seems to cause very rare crash when telling tooltip to show()
    // Appears to be same issue as ComboMenuWidget (see combomenuwidget.cpp for details)
    // though the tooltip bug is much more rare and I haven't yet found a way to reliably reproduce
    // Similarly, I removed Qt::Tooltip from flags and added WA_ShowWithoutActivating to prevent the tooltips
    // from stealing activation from the app and visibly updating the cursor type on every tooltip show
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
#endif
    setAttribute(Qt::WA_TranslucentBackground, true);
}

int ITooltip::leftTooltipMinY() const
{
    return TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE + TOOLTIP_TRIANGLE_HEIGHT*G_SCALE;
}

int ITooltip::leftTooltipMaxY() const
{
    return height_ - TOOLTIP_TRIANGLE_HEIGHT*G_SCALE - TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE;
}

int ITooltip::bottomTooltipMaxX() const
{
    return width_ - TOOLTIP_TRIANGLE_WIDTH*G_SCALE - TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE + additionalTailWidth();
}

int ITooltip::bottomTooltipMinX() const
{
    return TOOLTIP_OFFSET_ROUNDED_CORNER*G_SCALE + additionalTailWidth();
}

void ITooltip::startFadeInAnimation(int durationMs)
{
    setWindowOpacity(0.0);
    show();

    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(durationMs);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);

    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ITooltip::startFadeOutAnimation(int durationMs)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(durationMs);
    animation->setStartValue(windowOpacity());
    animation->setEndValue(0.0);

    connect(animation, &QPropertyAnimation::finished, this, &ITooltip::hide);
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
