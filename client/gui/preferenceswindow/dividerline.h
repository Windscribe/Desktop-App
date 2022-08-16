#ifndef DIVIDERLINE_H
#define DIVIDERLINE_H

#include "commongraphics/baseitem.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace PreferencesWindow {

class DividerLine : public CommonGraphics::BaseItem
{

public:
    explicit DividerLine(ScalableGraphicsObject *parent);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

private:
    static constexpr int DIVIDER_HEIGHT = 2;
};

} // namespace PreferencesWindow


#endif // DIVIDERLINE_H
