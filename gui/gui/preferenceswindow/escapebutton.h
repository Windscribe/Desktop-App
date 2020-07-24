#ifndef ESCAPEBUTTON_H
#define ESCAPEBUTTON_H

#include <QGraphicsObject>
#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace PreferencesWindow {

class EscapeButton : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit EscapeButton(ScalableGraphicsObject *parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setClickable(bool isClickable);

signals:
    void clicked();

private:
    IconButton *iconButton_;

};

} // namespace PreferencesWindow

#endif // CHECKBOXBUTTON_H
