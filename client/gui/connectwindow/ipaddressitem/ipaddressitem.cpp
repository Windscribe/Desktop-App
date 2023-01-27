#include "ipaddressitem.h"
#include "dpiscalemanager.h"

#include <QCursor>

IPAddressItem::IPAddressItem(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
    isValid_(false)
{
    for (int i = 0; i < 4; ++i)
    {
        octetItems_[i] = new OctetItem(this, &numbersPixmap_);
        connect(octetItems_[i], SIGNAL(needUpdate()), SLOT(doUpdate()));
        connect(octetItems_[i], SIGNAL(widthChanged()), SLOT(onOctetWidthChanged()));
    }
    onOctetWidthChanged();

    setGraphicsEffect(&blurEffect_);
    blurEffect_.setEnabled(false);
    setAcceptHoverEvents(true);
}

QRectF IPAddressItem::boundingRect() const
{
    return QRectF(0, 0, curWidth_, numbersPixmap_.height());
}

void IPAddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(255, 55, 255)));

    int x = 0;
    int y = 0;

    if (!isValid_)
    {
        numbersPixmap_.getNAPixmap()->draw(x + curWidth_ - numbersPixmap_.getNAPixmap()->width(), y, painter);
    }
    else
    {
        int curOffs = x;
        for(int i = 0; i < 4; ++i)
        {
            octetItems_[i]->draw(painter, curOffs, y);
            curOffs += octetItems_[i]->width();

            if (i != 3)
            {
                numbersPixmap_.getDotPixmap()->draw(curOffs, y, painter);
                curOffs += numbersPixmap_.dotWidth();
            }
        }
    }
}

void IPAddressItem::setIpAddress(const QString &ip, bool bWithAnimation)
{
    if (ip != ipAddress_)
    {
        ipAddress_ = ip;
        bool prevIsValid = isValid_;

        QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
        QRegularExpression ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
        isValid_ = ipRegex.match(ip).hasMatch();

        if (isValid_)
        {
            QStringList list = ipAddress_.split(".");

            for (int i = 0; i < 4; ++i)
            {
                octetItems_[i]->setOctetNumber(list[i].toInt(), prevIsValid && bWithAnimation);
            }

            if (!prevIsValid)
            {
                update();
            }
        }
        else
        {
            update();
        }
    }
}

void IPAddressItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    numbersPixmap_.rescale();

    // update ip
    QString curIp = ipAddress_;
    ipAddress_.clear();
    setIpAddress(curIp, false);

    for (int i = 0; i < 4; ++i)
    {
        octetItems_[i]->recalcSizes();
    }
    blurEffect_.setBlurRadius(defaultBlurRadius_ *  DpiScaleManager::instance().curDevicePixelRatio() * G_SCALE);
    onOctetWidthChanged();
}

void IPAddressItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    blurEffect_.setEnabled(!blurEffect_.isEnabled());
}

void IPAddressItem::onOctetWidthChanged()
{
    int newWidth = 0;
    for (int i = 0; i < 4; ++i)
    {
        newWidth += octetItems_[i]->width();
    }
    newWidth += numbersPixmap_.dotWidth() * 3;
    if (curWidth_ != newWidth)
    {
        curWidth_ = newWidth;
        emit widthChanged(curWidth_);
        update();
    }
}

void IPAddressItem::doUpdate()
{
    update();
}

void IPAddressItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::PointingHandCursor);
}

void IPAddressItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
}
