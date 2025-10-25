#include "iputilsmenu.h"

#include <QPainter>

#include "dpiscalemanager.h"

namespace ConnectWindow {

IpUtilsMenu::IpUtilsMenu(ScalableGraphicsObject *parent)
    : ScalableGraphicsObject(parent), isIpPinned_(false)
{
    favouriteButton_ = new IconButton(24, 24, "ip-utils/FAVOURITE", "", this);
    favouriteButton_->setUnhoverOpacity(0.7);
    favouriteButton_->setHoverOpacity(1.0);
    favouriteButton_->setClickableHoverable(true, true);
    connect(favouriteButton_, &IconButton::clicked, this, &IpUtilsMenu::favouriteClick);
    connect(favouriteButton_, &IconButton::hoverEnter, this, &IpUtilsMenu::favouriteHoverEnter);
    connect(favouriteButton_, &IconButton::hoverLeave, this, &IpUtilsMenu::favouriteHoverLeave);

    rotateButton_ = new IconButton(20, 20, "ip-utils/ROTATE", "", this);
    rotateButton_->setUnhoverOpacity(0.7);
    rotateButton_->setHoverOpacity(1.0);
    rotateButton_->setClickableHoverable(true, true);
    connect(rotateButton_, &IconButton::clicked, this, &IpUtilsMenu::rotateClick);
    connect(rotateButton_, &IconButton::hoverEnter, this, &IpUtilsMenu::rotateHoverEnter);
    connect(rotateButton_, &IconButton::hoverLeave, this, &IpUtilsMenu::rotateHoverLeave);

    closeButton_ = new IconButton(24, 24, "ip-utils/CLOSE", "", this);
    closeButton_->setUnhoverOpacity(0.7);
    closeButton_->setHoverOpacity(1.0);
    connect(closeButton_, &IconButton::clicked, this, &IpUtilsMenu::closeClick);

    updatePositions();
}

QRectF IpUtilsMenu::boundingRect() const
{
    return QRectF(0, 0, 104*G_SCALE, kHeight*G_SCALE);
}

void IpUtilsMenu::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void IpUtilsMenu::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void IpUtilsMenu::updatePositions()
{
    favouriteButton_->setPos(0, (kHeight*G_SCALE - favouriteButton_->boundingRect().height()) / 2);
    rotateButton_->setPos(40*G_SCALE, (kHeight*G_SCALE - rotateButton_->boundingRect().height()) / 2);
    closeButton_->setPos(80*G_SCALE, (kHeight*G_SCALE - closeButton_->boundingRect().height()) / 2);
}

void IpUtilsMenu::setIpAddress(const QString &address, bool isPinned)
{
    currentIpAddress_ = address;
    isIpPinned_ = isPinned;
    updateFavouriteIcon();
}

void IpUtilsMenu::updateFavouriteIcon()
{
    if (currentIpAddress_.isEmpty()) {
        favouriteButton_->setIcon("ip-utils/FAVOURITE");
        return;
    }

    if (isIpPinned_) {
        favouriteButton_->setIcon("ip-utils/FAVOURITE_SELECTED");
    } else {
        favouriteButton_->setIcon("ip-utils/FAVOURITE");
    }
}

} // namespace ConnectWindow
