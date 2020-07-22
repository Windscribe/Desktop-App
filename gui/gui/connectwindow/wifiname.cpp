#include "wifiname.h"

#include <QPainter>
#include "GraphicResources/imageresourcessvg.h"
#include "GraphicResources/fontmanager.h"

namespace ConnectWindow {

WiFiName::WiFiName(ScalableGraphicsObject *parent, const QString &wifiName) : ScalableGraphicsObject(parent),
    wifiName_(wifiName)
{
    trustButton_ = new CommonGraphics::TextIconButton(SPACE_BETWEEN_TEXT_AND_ARROW, "Secured", "FRWRD_ARROW_WHITE", this, false);
    trustButton_->setPos(width_ - trustButton_->getWidth(), 3);
    connect(trustButton_, SIGNAL(clicked()), this, SIGNAL(clicked()));

    font_ = *FontManager::instance().getFont(16, false);
    fontTrust_ = *FontManager::instance().getFont(16, true);
}

QRectF WiFiName::boundingRect() const
{
    return QRectF(0, 0, width_, 22);
}

void WiFiName::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setFont(font_);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(28, 0, 0, 0), wifiName_);
}

void WiFiName::setClickable(bool clickable)
{
    trustButton_->setClickable(clickable);
}

void WiFiName::setWiFiName(const QString &name)
{
    wifiName_ = name;
    update();
}

} //namespace ConnectWindow
