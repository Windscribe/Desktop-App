#include "staticipdeviceinfo.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "tooltips/tooltipcontroller.h"
#include "tooltips/tooltiptypes.h"

namespace GuiLocations {

StaticIPDeviceInfo::StaticIPDeviceInfo(QWidget *parent) : QWidget(parent), deviceName_(""), isNameElided_(false)
{
    // Enable mouse tracking to receive mouseMoveEvent even when no mouse button is pressed
    setMouseTracking(true);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &StaticIPDeviceInfo::onLanguageChanged);

    addButton_ = new CommonWidgets::IconButtonWidget("preferences/EXTERNAL_LINK_ICON", tr("Add"), this);
    addButton_->setFont(FontManager::instance().getFont(12, QFont::Normal));
    connect(addButton_, &CommonWidgets::IconButtonWidget::clicked, this, &StaticIPDeviceInfo::addStaticIpClicked);
}

void StaticIPDeviceInfo::setDeviceName(QString deviceName)
{
    deviceName_ = deviceName;
    update();
}

QSize StaticIPDeviceInfo::sizeHint() const
{
    return QSize(WINDOW_WIDTH * G_SCALE, height());
}

void StaticIPDeviceInfo::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

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

    QString name = deviceName_;
    QFont font = FontManager::instance().getFont(12, QFont::Normal);

    // Calculate remaining width after the add button, margins, and space between the button and text
    int remainingWidth = width() - WINDOW_MARGIN*2*G_SCALE - addButton_->width() - 16*G_SCALE;

    QFontMetrics metrics(font);
    int textWidth = metrics.horizontalAdvance(deviceName_);
    isNameElided_ = (textWidth > remainingWidth);
    if (isNameElided_) {
        name = metrics.elidedText(deviceName_, Qt::ElideRight, remainingWidth);
    }

    // Store the device name rectangle for tooltip positioning
    deviceNameRect_ = QRect(WINDOW_MARGIN*G_SCALE, 0, remainingWidth, height());

    // device name text
    painter.setOpacity(OPACITY_HALF);
    painter.setPen(Qt::white);
    painter.setFont(font);
    painter.drawText(deviceNameRect_, Qt::AlignVCenter, name);
}

void StaticIPDeviceInfo::onLanguageChanged()
{
    addButton_->setText(tr("Add"));
    update();
}

void StaticIPDeviceInfo::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePositions();
}

void StaticIPDeviceInfo::updatePositions()
{
    addButton_->setGeometry(width() - addButton_->width() - 16*G_SCALE, height()/2 - addButton_->height()/2, addButton_->width(), addButton_->height());
}

void StaticIPDeviceInfo::mouseMoveEvent(QMouseEvent *event)
{
    if (isNameElided_ && deviceNameRect_.contains(event->pos())) {
        // Show tooltip with full device name
        QPoint globalPos = mapToGlobal(QPoint(deviceNameRect_.x() + deviceNameRect_.width()/2, 0));

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_STATIC_IP_DEVICE_NAME);
        ti.x = globalPos.x();
        ti.y = globalPos.y();
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        ti.title = deviceName_;
        ti.parent = this;
        TooltipController::instance().showTooltipBasic(ti);
    } else {
        // Hide tooltip if mouse is not over the device name
        TooltipController::instance().hideTooltip(TOOLTIP_ID_STATIC_IP_DEVICE_NAME);
    }

    QWidget::mouseMoveEvent(event);
}

void StaticIPDeviceInfo::leaveEvent(QEvent *event)
{
    // Hide tooltip when mouse leaves the widget
    TooltipController::instance().hideTooltip(TOOLTIP_ID_STATIC_IP_DEVICE_NAME);
    QWidget::leaveEvent(event);
}

} // namespace GuiLocations
