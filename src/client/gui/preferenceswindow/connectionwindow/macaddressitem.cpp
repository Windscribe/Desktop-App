#include "macaddressitem.h"

#include <QPainter>

#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/network_utils/network_utils.h"

namespace PreferencesWindow {

MacAddressItem::MacAddressItem(ScalableGraphicsObject *parent, const QString &caption)
 : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), caption_(caption), isEditMode_(false)
{
    updateButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/REFRESH_ICON", "", this);
    connect(updateButton_, &IconButton::clicked, this, &MacAddressItem::onUpdateClick);
    updatePositions();
}

void MacAddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(12,  QFont::Normal);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              PREFERENCES_ITEM_Y*G_SCALE,
                                              -(2*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE,
                                              -PREFERENCES_MARGIN_Y*G_SCALE),
                      Qt::AlignLeft,
                      tr(caption_.toStdString().c_str()));

    painter->setOpacity(OPACITY_HALF);
    QString t;
    if (!text_.isEmpty())
    {
        t = NetworkUtils::formatMacAddress(text_);
    }
    else
    {
        t = "--";
    }

    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              PREFERENCES_ITEM_Y*G_SCALE,
                                              -(2*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE,
                                              -PREFERENCES_MARGIN_Y*G_SCALE),
                      Qt::AlignRight,
                      t);
}

void MacAddressItem::setMacAddress(const QString &macAddress)
{
    text_ = macAddress;
    update();
}

void MacAddressItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void MacAddressItem::onUpdateClick()
{
    emit cycleMacAddressClick();
}

void MacAddressItem::updatePositions()
{
    updateButton_->setPos(boundingRect().width() - (PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE);
}

void MacAddressItem::setCaption(const QString &caption)
{
    caption_ = caption;
    update();
}

} // namespace PreferencesWindow

