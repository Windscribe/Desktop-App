#include "macaddressitem.h"

#include <QPainter>
#include "../basepage.h"
#include "graphicresources/fontmanager.h"
#include <time.h>
#include "utils/utils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

MacAddressItem::MacAddressItem(ScalableGraphicsObject *parent, const QString &caption) :
    ScalableGraphicsObject(parent),
    caption_(caption),
    isEditMode_(false)
{
    dividerLine_ = new DividerLine(this, 264);

    btnUpdate_ = new IconButton(16, 16, "preferences/REFRESH_ICON", this);
    connect(btnUpdate_, SIGNAL(clicked()), SLOT(onUpdateClick()));
    updatePositions();
}

QRectF MacAddressItem::boundingRect() const
{
    return QRectF(0, 0, PAGE_WIDTH*G_SCALE, 43*G_SCALE);
}

void MacAddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->fillRect(boundingRect().adjusted(24*G_SCALE, 0, 0, 0), QBrush(QColor(16, 22, 40)));

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted((24 + 16)*G_SCALE, 0, 0, -3*G_SCALE), Qt::AlignVCenter, tr(caption_.toStdString().c_str()));

    painter->setOpacity(0.5 * initOpacity);
    QString t;
    if (!text_.isEmpty())
    {
        t = Utils::formatMacAddress(text_);
    }
    else
    {
        t = "--";
    }
    int rightOffs = -48*G_SCALE;
    painter->drawText(boundingRect().adjusted((24 + 16)*G_SCALE, 0, rightOffs, -3*G_SCALE), Qt::AlignRight | Qt::AlignVCenter, t);
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
    dividerLine_->setPos(24*G_SCALE, 40*G_SCALE);
    btnUpdate_->setPos(boundingRect().width() - btnUpdate_->boundingRect().width() - 16*G_SCALE,
                       (boundingRect().height() - dividerLine_->boundingRect().height() - btnUpdate_->boundingRect().height()) / 2);
}

} // namespace PreferencesWindow

