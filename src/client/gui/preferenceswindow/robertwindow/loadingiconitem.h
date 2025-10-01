#pragma once

#include <QMovie>
#include "commongraphics/scalablegraphicsobject.h"

class LoadingIconItem: public ScalableGraphicsObject
{
public:
    explicit LoadingIconItem(ScalableGraphicsObject *parent, QString file, int width, int height);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setOpacity(double opacity);
    void start();
    void stop();

private slots:
    void onFrameChanged();

private:
    QMovie *movie_;

    double opacity_;
    int width_;
    int height_;
};
