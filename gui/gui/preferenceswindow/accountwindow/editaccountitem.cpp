#include "editaccountitem.h"

#include <QPainter>
#include "GraphicResources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

EditAccountItem::EditAccountItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
{
    button_ = new IconButton(16, 16, "preferences/EXTERNAL_LINK_ICON", this);
    button_->setPos(boundingRect().width() - button_->boundingRect().width() - 16, (boundingRect().height() - button_->boundingRect().height()) / 2);
    connect(button_, SIGNAL(clicked()), SIGNAL(clicked()));
}

void EditAccountItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->setOpacity(0.5 * initialOpacity);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 1*G_SCALE, 0, -2*G_SCALE), Qt::AlignVCenter, tr("Edit Account Details"));
}

void EditAccountItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    button_->setPos(boundingRect().width() - button_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - button_->boundingRect().height()) / 2);
}


} // namespace PreferencesWindow
