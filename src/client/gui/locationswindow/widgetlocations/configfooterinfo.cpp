#include "configfooterinfo.h"

#include <QPainter>
#include <QMouseEvent>
#include <QFileDialog>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"

ConfigFooterInfo::IconButton::IconButton() : opacity(OPACITY_SEVENTY), is_hover(false) {}


ConfigFooterInfo::ConfigFooterInfo(QWidget *parent) : QAbstractButton(parent)
  , pressed_(false)
  , fullText_()
  , displayText_()
  , isDisplayTextRectHover_(false)
  , font_(FontManager::instance().getFont(16, QFont::Bold))
{
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    for (int i = 0; i < NUM_ICONS; ++i) {
        connect(&iconButtons_[i].opacityAnimation, &QVariantAnimation::valueChanged,
                [=](const QVariant &value) { onIconOpacityChanged(value, i); });
    }
}

QSize ConfigFooterInfo::sizeHint() const
{
    return QSize(WINDOW_WIDTH * G_SCALE, height());
}

QString ConfigFooterInfo::text() const
{
    return fullText_;
}

void ConfigFooterInfo::setText(const QString &text)
{
    fullText_ = text;
    updateDisplayText();
    update();
}

void ConfigFooterInfo::updateDisplayText()
{
    font_ = FontManager::instance().getFont(12, QFont::Normal);
    displayText_ = CommonGraphics::truncatedText(fullText_, font_,
        width() - (WINDOW_MARGIN * 4 + 40) * G_SCALE);
    displayTextRect_.setRect(WINDOW_MARGIN * G_SCALE, 0,
        CommonGraphics::textWidth(displayText_, font_), height() - BOTTOM_LINE_HEIGHT * G_SCALE);
}

void ConfigFooterInfo::updateButtonRects()
{
    const int kIconSize = 16 * G_SCALE;
    const int kIconYOffset = ((height() - BOTTOM_LINE_HEIGHT * G_SCALE) - kIconSize) / 2;

    iconButtons_[ICON_CLEAR].rect.setRect((WINDOW_WIDTH - WINDOW_MARGIN - 32) * G_SCALE - kIconSize,
        kIconYOffset, kIconSize, kIconSize);
    iconButtons_[ICON_CHOOSE].rect.setRect((WINDOW_WIDTH - WINDOW_MARGIN - 16) * G_SCALE,
        kIconYOffset, kIconSize, kIconSize);
}

void ConfigFooterInfo::updateScaling()
{
    updateDisplayText();
    updateButtonRects();
}

void ConfigFooterInfo::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    qreal initOpacity = painter.opacity();

    // background
    painter.setOpacity(0.05);
    painter.fillRect(QRect(0, 0, sizeHint().width(), height()), Qt::white);
    painter.setPen(QPen(Qt::white, 1*G_SCALE));
    painter.setBrush(Qt::NoBrush);
    painter.setOpacity(0.15);
    painter.drawLine(2*G_SCALE, 0, sizeHint().width() - 1*G_SCALE, 0);
    painter.setOpacity(1.0);

    const int kBottomLineHeight = BOTTOM_LINE_HEIGHT * G_SCALE;
    painter.fillRect(QRect(0, height() - kBottomLineHeight, sizeHint().width(), kBottomLineHeight),
                     FontManager::instance().getMidnightColor());

    if (!displayText_.isEmpty()) {
        font_ = FontManager::instance().getFont(12, QFont::Normal);
        painter.setOpacity(initOpacity * 0.5);
        painter.setPen(Qt::white);
        painter.setFont(font_);
        painter.drawText(displayTextRect_, Qt::AlignVCenter, displayText_);
    }

    QSharedPointer<IndependentPixmap> pixmap_clear =
        ImageResourcesSvg::instance().getIndependentPixmap("CLOSE_ICON");
    painter.setOpacity(initOpacity * iconButtons_[ICON_CLEAR].opacity);
    pixmap_clear->draw(iconButtons_[ICON_CLEAR].rect, &painter);

    QSharedPointer<IndependentPixmap> pixmap_choose =
        ImageResourcesSvg::instance().getIndependentPixmap("EDIT_ICON");
    painter.setOpacity(initOpacity * iconButtons_[ICON_CHOOSE].opacity);
    pixmap_choose->draw(iconButtons_[ICON_CHOOSE].rect, &painter);
}

void ConfigFooterInfo::leaveEvent(QEvent * /*event*/)
{
    isDisplayTextRectHover_ = false;
    TooltipController::instance().hideTooltip(TOOLTIP_ID_CONFIG_FOOTER);
}

void ConfigFooterInfo::mouseMoveEvent(QMouseEvent *event)
{
    bool is_any_icon_button_hover = false;
    for (int i = 0; i < NUM_ICONS; ++i) {
        const bool is_hover = iconButtons_[i].rect.contains(event->pos());
        if (is_hover)
            is_any_icon_button_hover = true;
        if (is_hover == iconButtons_[i].is_hover)
            continue;
        iconButtons_[i].is_hover = is_hover;
        if (is_hover) {
            startAnAnimation<double>(iconButtons_[i].opacityAnimation, iconButtons_[i].opacity,
                                     1.0, ANIMATION_SPEED_FAST);
        } else {
            startAnAnimation<double>(iconButtons_[i].opacityAnimation, iconButtons_[i].opacity,
                                     OPACITY_SEVENTY, ANIMATION_SPEED_FAST);
        }
    }
    setCursor(is_any_icon_button_hover ? Qt::PointingHandCursor : Qt::ArrowCursor);
    bool is_in_text = displayTextRect_.contains(event->pos());
    if (isDisplayTextRectHover_ != is_in_text) {
        isDisplayTextRectHover_ = is_in_text;
        if (is_in_text && fullText_ != displayText_) {
            QPoint pt = mapToGlobal(QPoint(WINDOW_MARGIN * G_SCALE
                + CommonGraphics::textWidth(displayText_, font_) / 2, 14 * G_SCALE));

            TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_CONFIG_FOOTER);
            ti.x = pt.x();
            ti.y = pt.y();
            ti.title = fullText_;
            ti.tailtype = TOOLTIP_TAIL_BOTTOM;
            ti.tailPosPercent = 0.5;
            ti.parent = this;
            TooltipController::instance().showTooltipBasic(ti);
        } else {
            TooltipController::instance().hideTooltip(TOOLTIP_ID_CONFIG_FOOTER);
        }
    }
}

void ConfigFooterInfo::mousePressEvent(QMouseEvent * /*event*/)
{
    pressed_ = true;
}

void ConfigFooterInfo::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    if (pressed_)
    {
        pressed_ = false;
        IconButton *clickedIconButton{ nullptr };

        if (iconButtons_[ICON_CLEAR].is_hover) {
            clickedIconButton = &iconButtons_[ICON_CLEAR];
            emit clearCustomConfigClicked();
        } else if (iconButtons_[ICON_CHOOSE].is_hover) {
            clickedIconButton = &iconButtons_[ICON_CHOOSE];
            emit addCustomConfigClicked();
        }
        if (clickedIconButton) {
            clickedIconButton->is_hover = false;
            startAnAnimation<double>(clickedIconButton->opacityAnimation,
                clickedIconButton->opacity, OPACITY_SEVENTY, ANIMATION_SPEED_FAST);
        }
    }
}

void ConfigFooterInfo::onIconOpacityChanged(const QVariant &value, int index)
{
    if (index < 0 || index >= NUM_ICONS) {
        WS_ASSERT(false);
        return;
    }
    iconButtons_[index].opacity = value.toDouble();
    update();
}
