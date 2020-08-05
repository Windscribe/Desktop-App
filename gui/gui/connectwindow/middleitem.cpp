#include "middleitem.h"

#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {

MiddleItem::MiddleItem(ScalableGraphicsObject *parent, const QString &ipAddress) : ScalableGraphicsObject(parent),
    ipAddress_(ipAddress), isSecured_(false), curTextColor_(Qt::white)
{
    ipAddressItem_ = new IPAddressItem(this);
    connect(ipAddressItem_, SIGNAL(needUpdate()), SLOT(onUpdate()));

    updateScaling();
}

QRectF MiddleItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH * G_SCALE, 20 * G_SCALE);
}

void MiddleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(255, 55, 255)));

    qreal initOpacity = painter->opacity();

    QFont *font = FontManager::instance().getFont(16, false);
    painter->setFont(*font);
    painter->setPen(curTextColor_);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, 0), tr("Firewall"));

    {
        IndependentPixmap *p;
        if (isSecured_)
        {
            p = ImageResourcesSvg::instance().getIndependentPixmap("IP_LOCK_SECURE");
        }
        else
        {
            p = ImageResourcesSvg::instance().getIndependentPixmap("IP_LOCK_UNSECURE");
        }
        painter->setOpacity(OPACITY_UNHOVER_ICON_STANDALONE * initOpacity);
        p->draw(301*G_SCALE, 2*G_SCALE, painter);
    }

    painter->setOpacity(initOpacity);
    QRectF rc = boundingRect().adjusted(0, 0, -38*G_SCALE, 0);
    ipAddressItem_->draw(painter, rc.right() - ipAddressItem_->width(), rc.top());
}

void MiddleItem::setTextColor(QColor color)
{
    curTextColor_ = color;
    update();
}

void MiddleItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    ipAddressItem_->setScale(G_SCALE);
}

void MiddleItem::setIpAddress(const QString &ipAddress)
{
    ipAddressItem_->setIpAddress(ipAddress);
    update();
}

void MiddleItem::setIsSecured(bool isSecured)
{
    isSecured_ = isSecured;
    update();
}

void MiddleItem::onUpdate()
{
    update();
}


} //namespace ConnectWindow
