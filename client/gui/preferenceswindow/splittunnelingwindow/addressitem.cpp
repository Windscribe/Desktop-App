#include "addressitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AddressItem::AddressItem(types::SplitTunnelingNetworkRoute route, ScalableGraphicsObject *parent)
  : CommonGraphics::BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), route_(route)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    deleteButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/DELETE_ICON", "", this, OPACITY_UNHOVER_ICON_STANDALONE, OPACITY_FULL);
    connect(deleteButton_, &IconButton::clicked, this, &AddressItem::deleteClicked);
    connect(deleteButton_, &IconButton::hoverEnter, this, &AddressItem::onDeleteButtonHoverEnter);
    connect(deleteButton_, &IconButton::hoverLeave, this, &AddressItem::hoverLeave);

    text_ = route.name;
    updateScaling();
}

void AddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setOpacity(OPACITY_FULL * initOpacity);
    painter->setPen(Qt::white);

    QFont *font = FontManager::instance().getFont(12, false);
    painter->setFont(*font);
    QFontMetrics fm(*font);
    QString elidedName = fm.elidedText(text_,
                                       Qt::TextElideMode::ElideRight,
                                       boundingRect().width() - (3*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -(2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      elidedName);
}

QString AddressItem::getAddressText()
{
    return text_;
}

types::SplitTunnelingNetworkRoute AddressItem::getNetworkRoute()
{
    return route_;
}

void AddressItem::setSelected(bool selected)
{
    selected_ = selected;
    deleteButton_->setSelected(selected);
    update();
}

void AddressItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void AddressItem::onDeleteButtonHoverEnter()
{
    if (!selected_)
    {
        selected_ = true;
        emit selectionChanged(true);
    }
}

void AddressItem::updatePositions()
{
    deleteButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN*G_SCALE, boundingRect().height()/2 - ICON_HEIGHT/2*G_SCALE);
}

void AddressItem::setClickable(bool clickable)
{
    deleteButton_->setClickable(clickable);
}

}
