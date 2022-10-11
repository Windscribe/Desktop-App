#include "loadingiconitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"

LoadingIconItem::LoadingIconItem(ScalableGraphicsObject *parent, QString file, int width, int height)
  : ScalableGraphicsObject(parent), width_(width), height_(height), opacity_(OPACITY_FULL)
{
    movie_ = new QMovie(file);
    movie_->setScaledSize(QSize(width*G_SCALE, height*G_SCALE));
    connect(movie_, &QMovie::frameChanged, this, &LoadingIconItem::onFrameChanged);
}

QRectF LoadingIconItem::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_*G_SCALE);
}

void LoadingIconItem::start()
{
    movie_->start();
}

void LoadingIconItem::stop()
{
    movie_->stop();
}

void LoadingIconItem::onFrameChanged()
{
    update();
}

void LoadingIconItem::setOpacity(double opacity)
{
    opacity_ = opacity;
}

void LoadingIconItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setOpacity(opacity_);
    painter->setPen(Qt::NoPen);
    painter->drawPixmap(0, 0, movie_->currentPixmap());
}