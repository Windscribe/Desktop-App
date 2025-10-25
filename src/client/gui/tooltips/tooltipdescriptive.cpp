#include "tooltipdescriptive.h"

#include <QPainter>
#include <QPainterPath>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

TooltipDescriptive::TooltipDescriptive(const TooltipInfo &info, QWidget *parent) : ITooltip(parent)
  , textTitle_(info.title)
{
    initWindowFlags();
    id_ = info.id;
    width_ = info.width;
    tailType_ = info.tailtype;
    tailPosPercent_ = info.tailPosPercent;
    showState_ = TOOLTIP_SHOW_STATE_INIT;
    animate_ = info.animate;
    animationSpeed_ = info.animationSpeed;

    QString ss = QString("QLabel { background-color: rgba(39, 49, 61, 96%); color: white }");
    labelDescr_.setParent(this);
    labelDescr_.setText(info.desc);
    labelDescr_.setStyleSheet(ss);
    labelDescr_.setAttribute(Qt::WA_TranslucentBackground);
    labelDescr_.setAlignment(Qt::AlignCenter);
    labelDescr_.setWordWrap(true);
    updateScaling();
}

void TooltipDescriptive::updateScaling()
{
    fontTitle_ = FontManager::instance().getFont(12, QFont::Bold);
    fontDescr_ = FontManager::instance().getFont(12,  QFont::Normal);
    labelDescr_.setFont(fontDescr_);

    QFontMetrics fm(fontTitle_);
    int descrTextPosY = MARGIN_HEIGHT*G_SCALE;
    if (textTitle_ != "") descrTextPosY += fm.height() + TITLE_DESC_SPACING*G_SCALE;

    labelDescr_.move(additionalTailWidth() + MARGIN_WIDTH*G_SCALE, descrTextPosY);
    labelDescr_.setFixedWidth(widthOfDescriptionLabel());

    recalcHeight();
}

TooltipInfo TooltipDescriptive::toTooltipInfo()
{
    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, id_);
    ti.title = textTitle_;
    ti.desc = labelDescr_.text();
    return ti;
}

void TooltipDescriptive::paintEvent(QPaintEvent * /*event*/)
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
    if (tailType_ != TOOLTIP_TAIL_NONE) {
        int tailLeftPtX = 0;
        int tailLeftPtY = 0;
        triangleLeftPointXY(tailLeftPtX, tailLeftPtY);

        if (tailType_ == TOOLTIP_TAIL_BOTTOM) {
            QPolygonF poly = trianglePolygonBottom(tailLeftPtX, tailLeftPtY);
            path.addPolygon(poly);
        } else if (tailType_ == TOOLTIP_TAIL_LEFT) {
            QPolygonF poly = trianglePolygonLeft(tailLeftPtX, tailLeftPtY);
            path.addPolygon(poly);
        }
    }
    painter.setPen(QColor(255,255,255, 255*0.15));
    painter.drawPath(path.simplified());

    // title
    if (textTitle_ != "") {
        QFontMetrics fm(fontTitle_);
        painter.setPen(Qt::white);
        painter.setFont(fontTitle_);
        painter.setOpacity(OPACITY_FULL);
        painter.drawText((additionalTailWidth() + width_/2) - fm.horizontalAdvance(textTitle_)/2, MARGIN_HEIGHT*G_SCALE + fm.height(), textTitle_);
    }

    // desc drawn implicitly
}

void TooltipDescriptive::recalcHeight()
{
    QFontMetrics fmTitle(fontTitle_);

    int newHeight = (MARGIN_HEIGHT * 2 * G_SCALE) + labelDescr_.heightForWidth(widthOfDescriptionLabel()) + additionalTailHeight();
    if (textTitle_ != "") newHeight += fmTitle.height() + TITLE_DESC_SPACING*G_SCALE;

    height_ = newHeight;

}

int TooltipDescriptive::widthOfDescriptionLabel() const
{
    return width_ - additionalTailWidth() - MARGIN_WIDTH* 2 * G_SCALE;
}
