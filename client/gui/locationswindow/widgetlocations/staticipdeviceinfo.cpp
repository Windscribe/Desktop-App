#include "staticipdeviceinfo.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace GuiLocations {


StaticIPDeviceInfo::StaticIPDeviceInfo(QWidget *parent) : QWidget(parent)
  , pressed_(false)
  , iconPath_("preferences/EXTERNAL_LINK_ICON")
  , addStaticTextWidth_(0)
  , addStaticTextHeight_(0)
  , deviceName_("")
  , font_(*FontManager::instance().getFont(16, true))
  , curTextOpacity_(OPACITY_UNHOVER_TEXT)
  , curIconOpacity_(OPACITY_UNHOVER_ICON_TEXT)
{
    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChange(QVariant)));
    connect(&iconOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onIconOpacityChange(QVariant)));
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
    QString addStaticText = tr("Add Static IP");

    QPainter painter(this);
    qreal initOpacity = painter.opacity();

    // background
    painter.fillRect(QRect(0,0, sizeHint().width(), height()),
        FontManager::instance().getCarbonBlackColor());

    const int kBottomLineHeight = BOTTOM_LINE_HEIGHT * G_SCALE;
    painter.fillRect(QRect(0, height() - kBottomLineHeight, sizeHint().width(), kBottomLineHeight),
        FontManager::instance().getMidnightColor());

    font_ = *FontManager::instance().getFont(16, true);

    // device name text
    painter.setOpacity(initOpacity * 0.5);
    painter.setPen(Qt::white);
    painter.setFont(font_);
    painter.drawText(QRect(WINDOW_MARGIN * G_SCALE, 14 * G_SCALE, CommonGraphics::textWidth(deviceName_, font_),
                           CommonGraphics::textHeight(font_)), Qt::AlignVCenter, deviceName_);

    // Add static IP text
    painter.setOpacity(initOpacity * curTextOpacity_);
    painter.setPen(Qt::white);
    painter.setFont(*FontManager::instance().getFont(16, false));
    int rightmostMargin = (WINDOW_WIDTH - 32) * G_SCALE;
    int rightmostStaticText = rightmostMargin - 5*G_SCALE;
    painter.drawText(QRect(0,0, rightmostStaticText, height()), Qt::AlignRight | Qt::AlignVCenter, addStaticText);

    // External Pixmap
    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);
    int iconHeight = pixmap->height();
    painter.setOpacity(initOpacity * curIconOpacity_);
    pixmap->draw((rightmostMargin),
                  height()/2 - iconHeight/2,
                  &painter);
}

void StaticIPDeviceInfo::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::PointingHandCursor);

    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void StaticIPDeviceInfo::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::PointingHandCursor);

    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_UNHOVER_ICON_TEXT, ANIMATION_SPEED_FAST);
}

void StaticIPDeviceInfo::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    pressed_ = true;
}

void StaticIPDeviceInfo::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressed_)
    {
        pressed_ = false;

        int x = geometry().x() + event->pos().x();
        int y = geometry().y() + event->pos().y();
        QPoint pt2 = QPoint(x, y);

        if (geometry().contains(pt2))
        {
            emit addStaticIpClicked();
        }
    }
}

void StaticIPDeviceInfo::onTextOpacityChange(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void StaticIPDeviceInfo::onIconOpacityChange(const QVariant &value)
{
    curIconOpacity_ = value.toDouble();
    update();
}

}
