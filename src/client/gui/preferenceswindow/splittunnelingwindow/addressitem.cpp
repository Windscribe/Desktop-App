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

    toggleButton_ = new ToggleButton(this);
    toggleButton_->setState(route_.active);
    connect(toggleButton_, &ToggleButton::stateChanged, this, &AddressItem::onToggleChanged);

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

    QFont font = FontManager::instance().getFont(14,  QFont::Normal);
    painter->setFont(font);
    QFontMetrics fm(font);
    QString elidedName = fm.elidedText(text_,
                                       Qt::TextElideMode::ElideRight,
                                       boundingRect().width() - (4*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE - toggleButton_->boundingRect().width());
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              0,
                                              -(3*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE - toggleButton_->boundingRect().width(),
                                              0),
                      Qt::AlignLeft | Qt::AlignVCenter,
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

bool AddressItem::isActive()
{
    return route_.active;
}

void AddressItem::onToggleChanged(bool checked)
{
    route_.active = checked;
    emit activeChanged(checked);
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
    deleteButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN_X*G_SCALE, boundingRect().height()/2 - ICON_HEIGHT/2*G_SCALE);
    toggleButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN_X*G_SCALE - toggleButton_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE,
                          (boundingRect().height() - toggleButton_->boundingRect().height()) / 2);
}

void AddressItem::setClickable(bool clickable)
{
    deleteButton_->setClickable(clickable);
}

}
