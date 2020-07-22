#ifndef VERTICALDIVIDERLINE_H
#define VERTICALDIVIDERLINE_H

#include "CommonGraphics/scalablegraphicsobject.h"

namespace CommonGraphics {


class VerticalDividerLine : public ScalableGraphicsObject
{
public:
    explicit VerticalDividerLine(ScalableGraphicsObject *parent, int height);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

private:
    int height_;
};

}

#endif // VERTICALDIVIDERLINE_H
