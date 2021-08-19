#ifndef VERTICALDIVIDERLINE_H
#define VERTICALDIVIDERLINE_H

#include "commongraphics/scalablegraphicsobject.h"

namespace CommonGraphics {


class VerticalDividerLine : public ScalableGraphicsObject
{
public:
    explicit VerticalDividerLine(ScalableGraphicsObject *parent, int height);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    int height_;
};

}

#endif // VERTICALDIVIDERLINE_H
