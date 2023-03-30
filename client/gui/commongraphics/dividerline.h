#ifndef DIVIDERLINE_H
#define DIVIDERLINE_H

#include "commongraphics/baseitem.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace CommonGraphics {

class DividerLine : public CommonGraphics::BaseItem
{

public:
    explicit DividerLine(ScalableGraphicsObject *parent, int width = PAGE_WIDTH, int startX = 16);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void updateScaling() override;

    void setOpacity(double opacity);

    static constexpr int DIVIDER_HEIGHT = 2;

private:
    double opacity_;
    int startX_;
};

} // namespace CommonGraphics

#endif // DIVIDERLINE_H
