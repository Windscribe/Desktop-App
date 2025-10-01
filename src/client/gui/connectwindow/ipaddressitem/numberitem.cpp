#include "numberitem.h"

#include <QTimer>
#include <QEasingCurve>

NumberItem::NumberItem(QObject *parent, NumbersPixmap *numbersPixmap) : QObject(parent),
    numbersPixmap_(numbersPixmap), curNum_(0), offs_(0), stopOffs_(0), stopAccelerationOffs_(0),
    prev_offs_(0), curSpeed_(0), prevSpeed_(0), state_(0), TIME_ACCELERATION(0), acceleration_(0),
    accelerationStopping_(0)
{
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &NumberItem::onTimer);
}

void NumberItem::setNumber(int num)
{
    if (num == -1)
    {
        num = 0;
    }

    if (timer_->isActive())
    {
        state_ = 1;

        qint64 roundedOffs = offs_ / numbersPixmap_->height();
        roundedOffs *= numbersPixmap_->height();
        roundedOffs += numbersPixmap_->height() * CNT_SCROLLING_NUMBERS_ON_ACCELERATION;

        int n = detectNumFromOffs(roundedOffs);
        roundedOffs += numbersPixmap_->height() * distBetweenNumbers(n, num);

        stopOffs_ = roundedOffs;
        stopAccelerationOffs_ = stopOffs_ - (numbersPixmap_->height() * CNT_SCROLLING_NUMBERS_ON_ACCELERATION);
    }
    else
    {
        offs_ = num * numbersPixmap_->height();
    }
}

void NumberItem::startAnimation(int timeAcceleration)
{
    if (!timer_->isActive())
    {
        TIME_ACCELERATION = timeAcceleration;
        acceleration_ = (double)CNT_SCROLLING_NUMBERS_ON_ACCELERATION * (double)numbersPixmap_->height() * 2.0 / (double)TIME_ACCELERATION;
        qint64 offs_corrected = (qint64)offs_ % ((qint64)numbersPixmap_->height() * (qint64)10);
        offs_ = offs_corrected;
        curSpeed_ = 0.0;
        state_ = 0;
        timer_->start(2);
        elapsed_.start();
        elapsedFrame_.start();
    }
}

void NumberItem::draw(QPainter *painter, int x, int y)
{
    qint64 offs_corrected = (qint64)offs_ % ((qint64)numbersPixmap_->height() * (qint64)10);
    numbersPixmap_->getPixmap()->draw(x, y, painter, 0, offs_corrected, numbersPixmap_->width(), numbersPixmap_->height());
}

int NumberItem::width() const
{
    return numbersPixmap_->width();
}

void NumberItem::onTimer()
{
    if (state_ == 0 || state_ == 1)
    {
        QEasingCurve c(QEasingCurve::Linear);
        curSpeed_ = c.valueForProgress(elapsed_.elapsed() / (double)TIME_ACCELERATION) * acceleration_;
        offs_ += curSpeed_ * elapsedFrame_.nsecsElapsed() / 1000000.0;

        if (state_ == 1 && offs_ >= stopAccelerationOffs_)
        {
            double S = (double)CNT_SCROLLING_NUMBERS_ON_ACCELERATION * (double)numbersPixmap_->height();
            accelerationStopping_ = 2 * (S - curSpeed_ * (double)TIME_ACCELERATION) / ((double)TIME_ACCELERATION);
            state_ = 2;
            elapsed_.start();
        }

        elapsedFrame_.start();
        emit needUpdate();
    }
    else
    {
        QEasingCurve c(QEasingCurve::Linear);

        double speed = curSpeed_ + c.valueForProgress(elapsed_.elapsed() / (double)TIME_ACCELERATION) * accelerationStopping_ * 0.95;
        offs_ += speed * elapsedFrame_.nsecsElapsed() / 1000000.0;
        if (offs_ >= stopOffs_ || speed <= 0.0)
        {
            offs_ = stopOffs_;
            timer_->stop();
        }
        elapsedFrame_.start();
        emit needUpdate();
    }
}


int NumberItem::detectNumFromOffs(double offs)
{
    qint64 offs_corrected = (qint64)offs % ((qint64)numbersPixmap_->height() * (qint64)10);
    return offs_corrected / numbersPixmap_->height();
}


int NumberItem::distBetweenNumbers(int n1, int n2)
{
    if (n1 <= n2)
    {
        return n2 - n1;
    }
    else
    {
        return 10 + (n2 - n1);
    }
}
