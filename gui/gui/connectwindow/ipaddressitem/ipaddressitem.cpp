#include "ipaddressitem.h"

IPAddressItem::IPAddressItem(QObject *parent) : QObject(parent),
    isValid_(false)
{
    for (int i = 0; i < 4; ++i)
    {
        octetItems_[i] = new OctetItem(this, &numbersPixmap_);
        connect(octetItems_[i], SIGNAL(needUpdate()), SIGNAL(needUpdate()));
        connect(octetItems_[i], SIGNAL(widthChanged()), SLOT(onOctetWidthChanged()));
    }
    onOctetWidthChanged();
}

void IPAddressItem::setIpAddress(const QString &ip, bool bWithAnimation)
{
    if (ip != ipAddress_)
    {
        ipAddress_ = ip;
        bool prevIsValid = isValid_;

        QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
        QRegExp ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
        isValid_ = ipRegex.exactMatch(ip);

        if (isValid_)
        {
            QStringList list = ip.split(".");

            for (int i = 0; i < 4; ++i)
            {
                octetItems_[i]->setOctetNumber(list[i].toInt(), prevIsValid && bWithAnimation);
            }

            if (!prevIsValid)
            {
                emit needUpdate();
            }
        }
        else
        {
            emit needUpdate();
        }
    }
}

int IPAddressItem::width() const
{
    return curWidth_;
}

int IPAddressItem::height() const
{
    return numbersPixmap_.height();
}

void IPAddressItem::draw(QPainter *painter, int x, int y)
{
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

void IPAddressItem::setScale(double /*scale*/)
{
    numbersPixmap_.rescale();

    // update ip
    QString curIp = ipAddress_;
    ipAddress_.clear();
    setIpAddress(curIp, false);

    for (int i = 0; i < 4; ++i)
    {
        octetItems_[i]->recalcSizes();
    }
    onOctetWidthChanged();
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
        emit needUpdate();
    }
}


