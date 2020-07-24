#include "viewlogitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ViewLogItem::ViewLogItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
{
    line_ = new DividerLine(this, 276);

    btnSendLog_ = new IconButton(16, 16, "preferences/SEND_LOG_ICON", this);
    connect(btnSendLog_, SIGNAL(hoverEnter()), SIGNAL(sendButtonHoverEnter()));
    connect(btnSendLog_, SIGNAL(hoverLeave()), SIGNAL(buttonHoverLeave()));
    connect(btnSendLog_, SIGNAL(clicked()), SIGNAL(sendLogClicked()));

    btnViewLog_ = new IconButton(16, 16, "preferences/VIEW_LOG_ICON", this);
    connect(btnViewLog_, SIGNAL(hoverEnter()), SIGNAL(viewButtonHoverEnter()));
    connect(btnViewLog_, SIGNAL(hoverLeave()), SIGNAL(buttonHoverLeave()));
    connect(btnViewLog_, SIGNAL(clicked()), SIGNAL(viewLogClicked()));

    updatePositions();
}

void ViewLogItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(25,255,100)));

    QFont *font = FontManager::instance().getFont(13, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, 0), Qt::AlignVCenter, tr("Log"));
}

void ViewLogItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    updatePositions();
}

void ViewLogItem::updatePositions()
{
    line_->setPos(24*G_SCALE, 47*G_SCALE);
    btnSendLog_->setPos(boundingRect().width() - btnSendLog_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - btnSendLog_->boundingRect().height()) / 2);
    btnViewLog_->setPos(boundingRect().width() - btnViewLog_->boundingRect().width()- btnSendLog_->boundingRect().width() - 32*G_SCALE, (boundingRect().height() - btnViewLog_->boundingRect().height()) / 2);
}

} // namespace PreferencesWindow
