#include "tooltipbasic.h"

#include <QPainter>
#include <QPainterPath>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

TooltipBasic::TooltipBasic(const TooltipInfo &info, QWidget *parent) : ITooltip(parent)
  , text_(info.title)
{
    initWindowFlags();
    id_ = info.id;
    tailType_ = info.tailtype;
    tailPosPercent_ = info.tailPosPercent;
    showState_ = TOOLTIP_SHOW_STATE_INIT;
    animate_ = info.animate;
    animationSpeed_ = info.animationSpeed;

    updateScaling();
}

void TooltipBasic::updateScaling()
{
    font_ = FontManager::instance().getFont(12,  QFont::Normal);
    recalcWidth();
    recalcHeight();
}

TooltipInfo TooltipBasic::toTooltipInfo()
{
    TooltipInfo ti(TOOLTIP_TYPE_BASIC, id_);
    ti.title = text_;
    ti.desc = "";
    return ti;
}

void TooltipBasic::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setOpacity(OPACITY_OVERLAY_BACKGROUND);
    painter.setPen(Qt::transparent);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(FontManager::instance().getCharcoalColor());

    // background
    QPainterPath path;
    QRect backgroundRect(additionalTailWidth(), 0, width_ - additionalTailWidth(), height_ - additionalTailHeight());
    path.addRoundedRect(backgroundRect, TOOLTIP_ROUNDED_RADIUS, TOOLTIP_ROUNDED_RADIUS);

    // tail
    if (tailType_ != TOOLTIP_TAIL_NONE)
    {
        int tailLeftPtX = 0;
        int tailLeftPtY = 0;
        triangleLeftPointXY(tailLeftPtX, tailLeftPtY);

        if (tailType_ == TOOLTIP_TAIL_BOTTOM)
        {
            QPolygonF poly = trianglePolygonBottom(tailLeftPtX, tailLeftPtY);
            path.addPolygon(poly);
        }
        else if (tailType_ == TOOLTIP_TAIL_LEFT)
        {
            QPolygonF poly = trianglePolygonLeft(tailLeftPtX, tailLeftPtY);
            path.addPolygon(poly);
        }
    }
    painter.setPen(QColor(255,255,255, 255*0.15));
    painter.drawPath(path.simplified());

    // text
    painter.setPen(Qt::white);
    painter.setFont(font_);
    painter.setOpacity(OPACITY_FULL);
    painter.drawText(MARGIN_WIDTH*G_SCALE + additionalTailWidth(), 20*G_SCALE, text_);

}

void TooltipBasic::recalcWidth()
{
    QFontMetrics fm(font_);
    int textWidthScaled = fm.horizontalAdvance(text_);
    int otherWidth = MARGIN_WIDTH * 2 * G_SCALE; // margins + spacing
    width_ = textWidthScaled + otherWidth + additionalTailWidth();
}

void TooltipBasic::recalcHeight()
{
    QFontMetrics fm(font_);
    height_ = (MARGIN_HEIGHT * 2 * G_SCALE) + fm.height() + additionalTailHeight();
}
