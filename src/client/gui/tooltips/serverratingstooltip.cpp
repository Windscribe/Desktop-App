#include "serverratingstooltip.h"

#include <QPainter>
#include <QPainterPath>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "utils/ws_assert.h"
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
    connect(rateUpButton_, &CommonWidgets::IconButtonWidget::clicked, this, &ServerRatingsTooltip::onRateUpButtonClicked);
    connect(rateUpButton_, &CommonWidgets::IconButtonWidget::hoverEnter, this, &ServerRatingsTooltip::onRateUpButtonHoverEnter);

    rateDownButton_ = new CommonWidgets::IconButtonWidget("RATE_SPEED_BAD_OFF", this);
    rateDownButton_->setUnhoverHoverOpacity(OPACITY_FULL, OPACITY_FULL);
    rateDownButton_->animateOpacityChange(OPACITY_FULL);
    connect(rateDownButton_, &CommonWidgets::IconButtonWidget::clicked, this, &ServerRatingsTooltip::onRateDownButtonClicked);
    connect(rateDownButton_, &CommonWidgets::IconButtonWidget::hoverEnter, this, &ServerRatingsTooltip::onRateDownButtonHoverEnter);

    hoverTimer_.setInterval(50);
    hoverTimer_.setSingleShot(false);
    connect(&hoverTimer_, &QTimer::timeout, this, &ServerRatingsTooltip::onHoverTimerTick);

    updateScaling();
}

void ServerRatingsTooltip::updateScaling()
{
    font_ = FontManager::instance().getFont(12,  QFont::Normal);
    recalcWidth();
    recalcHeight();
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
        WS_ASSERT(false);
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

void ServerRatingsTooltip::paintEvent(QPaintEvent * /*event*/)
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
    painter.drawText(MARGIN_WIDTH*G_SCALE + additionalTailWidth(), 20*G_SCALE, translatedText());
}

void ServerRatingsTooltip::onRateUpButtonHoverEnter()
{
    // need to manually activate to get cursor changes since we set WA_ShowWithoutActivating
    // This effectively delays activation until cursor is in the tooltip region,
    // so when the mainwindow deactivation hideAllTooltip happens, we don't close the tooltip
    activateWindow();
    setFocus();
}

void ServerRatingsTooltip::onRateDownButtonHoverEnter()
{
    // need to manually activate to get cursor changes since we set WA_ShowWithoutActivating
    // This effectively delays activation until cursor is in the tooltip region,
    // so when the mainwindow deactivation hideAllTooltip happens, we don't close the tooltip
    activateWindow();
    setFocus();
}

void ServerRatingsTooltip::recalcWidth()
{
    QFontMetrics fm(font_);
    int textWidthScaled = fm.horizontalAdvance(translatedText());
    int otherWidth = (MARGIN_WIDTH *2 + TEXT_BUTTON_SPACING + BUTTON_BUTTON_SPACING + BUTTON_WIDTH*2) * G_SCALE; // margins + spacing
    width_ = textWidthScaled + otherWidth + additionalTailWidth();
}

void ServerRatingsTooltip::recalcHeight()
{
    height_ = (MARGIN_HEIGHT*2 + BUTTON_HEIGHT) * G_SCALE + additionalTailHeight();
}

void ServerRatingsTooltip::updatePositions()
{
    QFontMetrics fm(font_);
    int textWidth = fm.horizontalAdvance(translatedText());
    int padding = MARGIN_WIDTH *G_SCALE + TEXT_BUTTON_SPACING*G_SCALE + additionalTailWidth();

    rateUpButton_->updateSize();
    rateDownButton_->updateSize();
    rateUpButton_->setGeometry(textWidth + padding, MARGIN_HEIGHT*G_SCALE, rateUpButton_->width(), rateUpButton_->height());
    rateDownButton_->setGeometry(textWidth + padding + rateUpButton_->width() + MARGIN_WIDTH*G_SCALE,
                                 MARGIN_HEIGHT*G_SCALE,
                                 rateDownButton_->width(), rateDownButton_->height());
}

const QString ServerRatingsTooltip::translatedText()
{
    return tr("Rate speed");
}

