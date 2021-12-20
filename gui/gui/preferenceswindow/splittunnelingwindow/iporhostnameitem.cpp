#include "iporhostnameitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

IpOrHostnameItem::IpOrHostnameItem(ProtoTypes::SplitTunnelingNetworkRoute route, ScalableGraphicsObject *parent) : BaseItem(parent, 50)
    , route_(route)
{
    deleteButton_ = new IconButton(16, 16, "preferences/DELETE_ICON", "", this, OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    deleteButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    deleteButton_->setHoverOpacity(OPACITY_FULL);
    connect(deleteButton_, SIGNAL(clicked()), SIGNAL(deleteClicked()));
    connect(deleteButton_, SIGNAL(hoverEnter()), SLOT(onDeleteButtonHoverEnter()));
    connect(deleteButton_, SIGNAL(hoverLeave()), SIGNAL(hoverLeave()));

    line_ = new DividerLine(this, 276);

    text_ = QString::fromStdString(route.name());
    updateScaling();
}

void IpOrHostnameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setOpacity(OPACITY_FULL * initOpacity);
    painter->setPen(Qt::white);
    painter->setFont(*FontManager::instance().getFont(12, false));
    painter->drawText(boundingRect().adjusted(20*G_SCALE, 16*G_SCALE, -48*G_SCALE, 0), text_);
}

QString IpOrHostnameItem::getIpOrHostnameText()
{
    return text_;
}

ProtoTypes::SplitTunnelingNetworkRoute IpOrHostnameItem::getNetworkRoute()
{
    return route_;
}

void IpOrHostnameItem::setSelected(bool selected)
{
    selected_ = selected;
    deleteButton_->setSelected(selected);
    update();
}

void IpOrHostnameItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    updatePositions();
}

void IpOrHostnameItem::onDeleteButtonHoverEnter()
{
    if (!selected_)
    {
        selected_ = true;
        emit selectionChanged(true);
    }
}

void IpOrHostnameItem::updatePositions()
{
    deleteButton_->setPos(boundingRect().width() - 32*G_SCALE, boundingRect().height()/2 - 8*G_SCALE);
    line_->setPos(24*G_SCALE, 47*G_SCALE);
}

}
