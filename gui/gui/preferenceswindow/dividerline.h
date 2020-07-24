#ifndef DIVIDERLINE_H
#define DIVIDERLINE_H

#include "commongraphics/scalablegraphicsobject.h"

namespace PreferencesWindow {

class DividerLine : public ScalableGraphicsObject
{

public:
    explicit DividerLine(ScalableGraphicsObject *parent, int width);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

private:
    int width_;

};

} // namespace PreferencesWindow


#endif // DIVIDERLINE_H
