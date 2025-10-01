#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace CommonGraphics {

class DividerLine : public CommonGraphics::BaseItem
{

public:
    explicit DividerLine(ScalableGraphicsObject *parent, int width = PAGE_WIDTH, int startX = 0);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setOpacity(double opacity);

    static constexpr int DIVIDER_HEIGHT = 1;

private:
    double opacity_;
    int startX_;
};

} // namespace CommonGraphics
