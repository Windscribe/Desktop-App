#include "octetitem.h"

#include <QTimer>
#include "utils/utils.h"
#include "dpiscalemanager.h"

OctetItem::OctetItem(QObject *parent, NumbersPixmap *numbersPixmap) : QObject(parent), numbersPixmap_(numbersPixmap)
{
    for (int i = 0; i < 3; ++i)
    {
        numberItem_[i] = new NumberItem(this, numbersPixmap);
        connect(numberItem_[i], &NumberItem::needUpdate, this, &OctetItem::needUpdate);
        curNums_[i] = 0;
    }
    curWidth_ = calcWidth();

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &OctetItem::onTimer);
}

void OctetItem::setOctetNumber(int num, bool bWithAnimation)
{
    if (bWithAnimation)
    {
        for (int i = 0; i < 3; ++i)
        {
            numberItem_[i]->startAnimation(Utils::generateIntegerRandom(200, 600));
        }
        timer_->start(2);
        elapsed_.start();
    }

    int n[3];
    setNums(num, n[0], n[1], n[2]);

    for (int i = 0; i < 3; ++i)
    {
        numberItem_[i]->setNumber(n[i]);
        curNums_[i] = n[i];
    }
    oldWidth_ = curWidth_;
    newWidth_ = calcWidth();

    if (!bWithAnimation)
    {
        curWidth_ = newWidth_;
        emit widthChanged();
    }
}

void OctetItem::draw(QPainter *painter, int x, int y)
{
    QPixmap pixmap(QSize(numbersPixmap_->width() * 3, numbersPixmap_->height()) * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    pixmap.fill(QColor(Qt::transparent));

    {
        QPainter p(&pixmap);
        int curOffs = 0;
        for(int i = 0; i < 3; ++i)
        {
            numberItem_[i]->draw(&p, curOffs, 0);
            curOffs += numberItem_[i]->width();
        }
    }

    painter->drawPixmap(x, y, pixmap, (pixmap.width() - curWidth_* DpiScaleManager::instance().curDevicePixelRatio()), 0, curWidth_* DpiScaleManager::instance().curDevicePixelRatio(),
                        pixmap.height());
}

int OctetItem::width() const
{
    return curWidth_;
}

void OctetItem::recalcSizes()
{
    curWidth_ = calcWidth();
}

void OctetItem::onTimer()
{
    double d = elapsed_.elapsed() / 400.0;

    curWidth_ = oldWidth_ + (newWidth_ - oldWidth_) * d;

    if (d >= 1.0)
    {
        curWidth_ = newWidth_;
        timer_->stop();

    }
    //emit needUpdate();
    emit widthChanged();
}

void OctetItem::setNums(int input, int &n1, int &n2, int &n3)
{
    n1 = input / 100;
    n2 = (input - n1 * 100) / 10;
    n3 = input % 10;

    if (n1 == 0)
    {
        n1 = -1;

        if (n2 == 0)
        {
            n2 = -1;
        }
    }
}

int OctetItem::calcWidth()
{
    int w = 0;
    bool isFirstZero = true;
    for(int i = 0; i < 3; ++i)
    {
        if (curNums_[i] != -1 && isFirstZero)
        {
            isFirstZero = false;
        }

        if (!isFirstZero)
        {
            w += numberItem_[i]->width();
        }
    }

    return w;
}
