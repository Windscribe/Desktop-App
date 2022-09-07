#ifndef DIVIDERLINE_H
#define DIVIDERLINE_H

#include "commongraphics/baseitem.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace CommonGraphics {

class DividerLine : public CommonGraphics::BaseItem
{

public:
    explicit DividerLine(ScalableGraphicsObject *parent, int width = PAGE_WIDTH);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setOpacity(double opacity);

private:
    static constexpr int DIVIDER_HEIGHT = 2;

    double opacity_;
};

} // namespace CommonGraphics

#endif // DIVIDERLINE_H
