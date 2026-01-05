#include "dnsdomainaddressitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

DnsDomainAddressItem::DnsDomainAddressItem(const QString &address, ScalableGraphicsObject *parent)
  : CommonGraphics::BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), address_(address)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    deleteButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/DELETE_ICON", "", this, OPACITY_UNHOVER_ICON_STANDALONE, OPACITY_FULL);
    connect(deleteButton_, &IconButton::clicked, this, &DnsDomainAddressItem::deleteClicked);
    connect(deleteButton_, &IconButton::hoverEnter, this, &DnsDomainAddressItem::onDeleteButtonHoverEnter);
    connect(deleteButton_, &IconButton::hoverLeave, this, &DnsDomainAddressItem::hoverLeave);

    updateScaling();
}

void DnsDomainAddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setOpacity(OPACITY_FULL * initOpacity);
    painter->setPen(Qt::white);

    QFont font = FontManager::instance().getFont(14,  QFont::Normal);
    painter->setFont(font);
    QFontMetrics fm(font);
    QString elidedName = fm.elidedText(address_,
                                       Qt::TextElideMode::ElideRight,
                                       boundingRect().width() - (3*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              0,
                                              -(2*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE,
                                              0),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      elidedName);
}

QString DnsDomainAddressItem::getAddressText()
{
    return address_;
}

void DnsDomainAddressItem::setSelected(bool selected)
{
    selected_ = selected;
    deleteButton_->setSelected(selected);
    update();
}

void DnsDomainAddressItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void DnsDomainAddressItem::onDeleteButtonHoverEnter()
{
    if (!selected_)
    {
        selected_ = true;
        emit selectionChanged(true);
    }
}

void DnsDomainAddressItem::updatePositions()
{
    deleteButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN_X*G_SCALE, boundingRect().height()/2 - ICON_HEIGHT/2*G_SCALE);
}

void DnsDomainAddressItem::setClickable(bool clickable)
{
    deleteButton_->setClickable(clickable);
}

}
