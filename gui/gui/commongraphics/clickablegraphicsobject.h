#ifndef CLICKABLEGRAPHICSOBJECT_H
#define CLICKABLEGRAPHICSOBJECT_H

#include "CommonGraphics/scalablegraphicsobject.h"

class ClickableGraphicsObject : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ClickableGraphicsObject(ScalableGraphicsObject *parent);

    virtual QRectF boundingRect() const = 0;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) = 0;

    virtual bool isClickable();
    virtual void setClickable(bool clickable); // TODO: split clickablehoverable into two independent functions
    virtual void setClickableHoverable(bool clickable, bool hoverable);

    virtual bool isSelected();
    virtual void setSelected(bool selected);
    virtual void setStickySelection(bool stickySelection);

signals:
    void clicked();
    void hoverEnter();
    void hoverLeave();
    void selectionChanged(bool selected);

protected slots:
    virtual void onLanguageChanged();

protected:
    bool selected_;
    bool stickySelection_;
    bool clickable_;
    bool hoverable_;

    bool pressed_;
    bool hovered_;

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

};

#endif // CLICKABLEGRAPHICSOBJECT_H
