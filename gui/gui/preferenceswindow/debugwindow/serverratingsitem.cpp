#include "serverratingsitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ServerRatingsItem::ServerRatingsItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
{
    line_ = new DividerLine(this, 276);

    btnClear_ = new IconButton(16, 16, "preferences/DELETE_ICON", this);
    connect(btnClear_, SIGNAL(hoverEnter()), SIGNAL(clearButtonHoverEnter()));
    connect(btnClear_, SIGNAL(hoverLeave()), SIGNAL(clearButtonHoverLeave()));
    connect(btnClear_, SIGNAL(clicked()), SIGNAL(clicked()));

    updatePositions();
}

void ServerRatingsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(25,255,100)));

    QFont *font = FontManager::instance().getFont(13, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, 0), Qt::AlignVCenter, tr("Server Ratings"));
}

void ServerRatingsItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    updatePositions();
}

void ServerRatingsItem::updatePositions()
{
    line_->setPos(24*G_SCALE, 47*G_SCALE);
    btnClear_->setPos(boundingRect().width() - btnClear_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - btnClear_->boundingRect().height()) / 2);
}

} // namespace PreferencesWindow
