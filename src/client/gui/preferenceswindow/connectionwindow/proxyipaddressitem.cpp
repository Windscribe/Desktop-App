#include "proxyipaddressitem.h"

#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "dpiscalemanager.h"
#include "commongraphics/basepage.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "tooltips/tooltipcontroller.h"


namespace PreferencesWindow {

ProxyIpAddressItem::ProxyIpAddressItem(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)
{
    copyButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/COPY_ICON", "", this);
    connect(copyButton_, &IconButton::clicked, this, &ProxyIpAddressItem::onCopyClick);

    updatePositions();
}

void ProxyIpAddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(14,  QFont::Normal);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              0,
                                              -(ICON_WIDTH + 2*PREFERENCES_MARGIN_X)*G_SCALE,
                                              0),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      tr("IP"));

    painter->setOpacity(OPACITY_HALF);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              0,
                                              -(ICON_WIDTH + 2*PREFERENCES_MARGIN_X)*G_SCALE,
                                              0),
                      Qt::AlignRight | Qt::AlignVCenter,
                      strIP_);
}

void ProxyIpAddressItem::setIP(const QString &strIP)
{
    strIP_ = strIP;
    update();
}

void ProxyIpAddressItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void ProxyIpAddressItem::onCopyClick()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_PROXY_IP_COPIED); // multi-clicks will keep the tooltip alive

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(strIP_);

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(copyButton_->scenePos()));
    int posX = globalPt.x() + 8*G_SCALE;
    int posY = globalPt.y() - 3*G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_PROXY_IP_COPIED);
    ti.x = posX;
    ti.y = posY;
    ti.title = tr("Copied");
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 1;
    ti.delay = 0;
    if (scene() && !scene()->views().isEmpty()) {
        ti.parent = scene()->views().first()->viewport();
    }
    TooltipController::instance().showTooltipBasic(ti);

    QTimer::singleShot(TOOLTIP_TIMEOUT, [](){
        TooltipController::instance().hideTooltip(TOOLTIP_ID_PROXY_IP_COPIED);
    });
}

void ProxyIpAddressItem::updatePositions()
{
    copyButton_->setPos(boundingRect().width() - (ICON_WIDTH + PREFERENCES_MARGIN_X)*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE);
}

} // namespace PreferencesWindow

