#ifndef ESCAPEBUTTON_H
#define ESCAPEBUTTON_H

#include <QGraphicsObject>
#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace CommonGraphics {

class EscapeButton : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit EscapeButton(ScalableGraphicsObject *parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setClickable(bool isClickable);
    int getSize() const { return BUTTON_SIZE; }

signals:
    void clicked();

private:
    IconButton *iconButton_;

    static constexpr int BUTTON_SIZE = 24;
};

} // namespace CommonGraphics

#endif // ESCAPEBUTTON_H
