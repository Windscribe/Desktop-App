#include "proxyipaddressitem.h"

#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "../basepage.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"


namespace PreferencesWindow {

ProxyIpAddressItem::ProxyIpAddressItem(ScalableGraphicsObject *parent, bool isDrawFullBottomDivider) : ScalableGraphicsObject(parent)
{
    line_ = new DividerLine(this, isDrawFullBottomDivider ? 276 : 264);

    btnCopy_ = new IconButton(16, 16, "preferences/COPY_ICON", this);
    connect(btnCopy_, SIGNAL(clicked()), SLOT(onCopyClick()));

    updatePositions();
}

QRectF ProxyIpAddressItem::boundingRect() const
{
    return QRectF(0, 0, PAGE_WIDTH*G_SCALE, 43*G_SCALE);
}

void ProxyIpAddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    painter->fillRect(boundingRect().adjusted(24*G_SCALE, 0, 0, 0), QBrush(QColor(16, 22, 40)));

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(40*G_SCALE, 0, 0, -3*G_SCALE), Qt::AlignVCenter, tr("Connect To"));

    painter->setOpacity(0.5 * initialOpacity);
    painter->drawText(boundingRect().adjusted(40*G_SCALE, 0, -48*G_SCALE, -3*G_SCALE), Qt::AlignRight | Qt::AlignVCenter, strIP_);
}

void ProxyIpAddressItem::setIP(const QString &strIP)
{
    strIP_ = strIP;
    update();
}

void ProxyIpAddressItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void ProxyIpAddressItem::onCopyClick()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_PROXY_IP_COPIED); // multi-clicks will keep the tooltip alive

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(strIP_);

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(btnCopy_->scenePos()));
    int posX = globalPt.x() + 8*G_SCALE;
    int posY = globalPt.y() - 3*G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_PROXY_IP_COPIED);
    ti.x = posX;
    ti.y = posY;
    ti.title = tr("Copied");
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 1;
    ti.delay = 0;
    TooltipController::instance().showTooltipBasic(ti);

    QTimer::singleShot(TOOLTIP_TIMEOUT, [](){
        TooltipController::instance().hideTooltip(TOOLTIP_ID_PROXY_IP_COPIED);
    });
}

void ProxyIpAddressItem::updatePositions()
{
    line_->setPos(24*G_SCALE, 40*G_SCALE);
    btnCopy_->setPos(boundingRect().width() - btnCopy_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - line_->boundingRect().height() - btnCopy_->boundingRect().height()) / 2);
}

} // namespace PreferencesWindow

