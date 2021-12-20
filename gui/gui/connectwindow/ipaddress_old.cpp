#include "ipaddress.h"

#include <QPainter>
#include "GraphicResources/imageresourcessvg.h"
#include "GraphicResources/fontmanager.h"
#include "CommonGraphics/commongraphics.h"

namespace ConnectWindow {

IPAddress::IPAddress(QGraphicsObject *parent, const QString &ipAddress) : QGraphicsObject(parent),
    ipAddress_(ipAddress), isSecured_(false)
{
    font_ = *FontManager::instance().getFont(16, false);
    curTextColor_ = Qt::white;
    lockColorDark_ = false;
}

QRectF IPAddress::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH, 20);
}

void IPAddress::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(255, 55, 255)));

    qreal initOpacity = painter->opacity();

    painter->setFont(font_);
    painter->setPen(curTextColor_);
    painter->drawText(boundingRect().adjusted(0, 0, 0, 0), tr("Firewall"));

    {
        QPixmap *p;
        if (isSecured_)
        {
            if (lockColorDark_)
            {
                p = ImageResourcesSvg::instance().getPixmap("SECURE_LOCK_ICON");
            }
            else
            {
                p = ImageResourcesSvg::instance().getPixmap("LOCK_SECURE_WHITE");
            }
        }
        else
        {
            if (lockColorDark_)
            {
                p = ImageResourcesSvg::instance().getPixmap("UNSECURE_LOCK_ICON");
            }
            else
            {
                p = ImageResourcesSvg::instance().getPixmap("LOCK_UNSECURE_WHITE");
            }
        }
        painter->setOpacity(OPACITY_UNHOVER_ICON_STANDALONE * initOpacity);
        painter->drawPixmap(286, 2, *p);
    }

    painter->setOpacity(0.5 * initOpacity);
    painter->drawText(boundingRect().adjusted(0, 0, -54, 0), Qt::AlignRight, ipAddress_);

}

void IPAddress::setTextColor(QColor color)
{
    curTextColor_ = color;
    update();
}

void IPAddress::setLockColorDark(bool dark)
{
    lockColorDark_ = dark;
    update();
}

void IPAddress::setIpAddress(const QString &ipAddress)
{
    ipAddress_ = ipAddress;
    update();
}

void IPAddress::setIsSecured(bool isSecured)
{
    isSecured_ = isSecured;
    update();
}

} //namespace ConnectWindow
