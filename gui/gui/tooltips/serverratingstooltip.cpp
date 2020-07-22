#include "serverratingstooltip.h"

#include <QPainter>
#include "CommonGraphics/commongraphics.h"
#include "GraphicResources/fontmanager.h"
#include "dpiscalemanager.h"

ServerRatingsTooltip::ServerRatingsTooltip(QWidget *parent) : ITooltip(parent)
  , state_(SERVER_RATING_NONE)
{
    initWindowFlags();
    tailType_ = TOOLTIP_TAIL_BOTTOM;
    tailPosPercent_ = 0.9;
    showState_ = TOOLTIP_SHOW_STATE_INIT;

    rateUpButton_ = new CommonWidgets::IconButtonWidget("RATE_SPEED_GOOD_OFF", this);
    rateUpButton_->setUnhoverHoverOpacity(OPACITY_FULL, OPACITY_FULL);
    rateUpButton_->animateOpacityChange(OPACITY_FULL);
    connect(rateUpButton_, SIGNAL(clicked()), SLOT(onRateUpButtonClicked()));

    rateDownButton_ = new CommonWidgets::IconButtonWidget("RATE_SPEED_BAD_OFF", this);
    rateDownButton_->setUnhoverHoverOpacity(OPACITY_FULL, OPACITY_FULL);
    rateDownButton_->animateOpacityChange(OPACITY_FULL);
    connect(rateDownButton_, SIGNAL(clicked()), SLOT(onRateDownButtonClicked()));

    hoverTimer_.setInterval(50);
    hoverTimer_.setSingleShot(false);
    connect(&hoverTimer_, SIGNAL(timeout()), SLOT(onHoverTimerTick()));

    updateScaling();
}

void ServerRatingsTooltip::updateScaling()
{
    font_ = *FontManager::instance().getFont(12, false);
    recalcWidth();
    recalcHeight();
    rateUpButton_->setGeometry(  0, 0, buttonWidth_*G_SCALE, buttonHeight_*G_SCALE);
    rateDownButton_->setGeometry(0, 0, buttonWidth_*G_SCALE, buttonHeight_*G_SCALE);
    updatePositions();
}

bool ServerRatingsTooltip::isHovering()
{
    bool result = false;

    QPoint pt = QCursor::pos();

    if (geometry().contains(pt))
    {
        result = true;
    }

    return result;
}

void ServerRatingsTooltip::setRatingState(ServerRatingState ratingState)
{
    if (ratingState == SERVER_RATING_GOOD)
    {
        rateUpButton_->setImage("RATE_SPEED_GOOD_ON");
        rateDownButton_->setImage("RATE_SPEED_BAD_OFF");
        state_ = ratingState;
    }
    else if (ratingState == SERVER_RATING_BAD)
    {
        rateUpButton_->setImage("RATE_SPEED_GOOD_OFF");
        rateDownButton_->setImage("RATE_SPEED_BAD_ON");
        state_ = ratingState;
    }
    else if (ratingState == SERVER_RATING_NONE)
    {
        rateUpButton_->setImage("RATE_SPEED_GOOD_OFF");
        rateDownButton_->setImage("RATE_SPEED_BAD_OFF");
        state_ = ratingState;
    }
    else
    {
        Q_ASSERT(false);
    }
}

void ServerRatingsTooltip::startHoverTimer()
{
    hoverTimer_.start();
}

void ServerRatingsTooltip::stopHoverTimer()
{
    hoverTimer_.stop();
}

TooltipInfo ServerRatingsTooltip::toTooltipInfo()
{
    TooltipInfo ti(TOOLTIP_TYPE_INTERACTIVE, TOOLTIP_ID_SERVER_RATINGS);
    ti.title = translatedText();
    ti.desc = "";
    return ti;
}

void ServerRatingsTooltip::onRateUpButtonClicked()
{
    if (state_ == SERVER_RATING_NONE)
    {
        state_ = SERVER_RATING_GOOD;
        rateUpButton_->setImage("RATE_SPEED_GOOD_ON");
        emit rateUpClicked();
    }
}

void ServerRatingsTooltip::onRateDownButtonClicked()
{
    if (state_ == SERVER_RATING_NONE)
    {
        state_ = SERVER_RATING_BAD;
        rateDownButton_->setImage("RATE_SPEED_BAD_ON");
        emit rateDownClicked();
    }
}

void ServerRatingsTooltip::onHoverTimerTick()
{
    // There appears to be a QTBUG where leaveEvent is sent consecutively after enterEvent on MacOS after both enter and leave events
    // instead we manually listen for a more relaible leave signal here
    if (!isHovering())
    {
        hoverTimer_.stop();
        hide();
    }
}

void ServerRatingsTooltip::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setOpacity(OPACITY_OVERLAY_BACKGROUND);
    painter.setPen(Qt::transparent);
    painter.setRenderHint(QPainter::Antialiasing);

    // shadow // removed because shadow looks a bit grainy
//    QColor shadowColor = FontManager::instance().getMidnightColor();
//    QPixmap shape = getCurrentPixmapShape();
//    QPixmap shadow = MakeCustomShadow::makeShadowForPixmap(shape, SHADOW_SIZE, SHADOW_SIZE, shadowColor);
//    painter.drawPixmap(0,0,shadow);

    // background
    painter.setBrush(FontManager::instance().getCharcoalColor());
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
    painter.drawText(marginWidth_*G_SCALE + additionalTailWidth(), 20*G_SCALE, translatedText());
}

void ServerRatingsTooltip::recalcWidth()
{
    QFontMetrics fm(font_);
    int textWidthScaled = fm.width(translatedText());
    int otherWidth = (marginWidth_*2 + textButtonSpacing_ + buttonButtonSpacing_ + buttonWidth_*2) * G_SCALE; // margins + spacing
    width_ = textWidthScaled + otherWidth + additionalTailWidth();
}

void ServerRatingsTooltip::recalcHeight()
{
    height_ = (marginHeight_*2 + buttonHeight_) * G_SCALE + additionalTailHeight();
}

void ServerRatingsTooltip::updatePositions()
{
    QFontMetrics fm(font_);
    int textWidth = fm.width(translatedText());
    int padding = marginWidth_*G_SCALE + textButtonSpacing_*G_SCALE + additionalTailWidth();
    rateUpButton_->move(textWidth + padding, marginHeight_*G_SCALE);
    rateDownButton_->move(textWidth + padding + buttonWidth_*G_SCALE + marginWidth_*G_SCALE, marginHeight_*G_SCALE);
}

const QString ServerRatingsTooltip::translatedText()
{
    return tr("Rate speed");
}

