#pragma once

#include "commongraphics/scalablegraphicsobject.h"

class ClickableGraphicsObject : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ClickableGraphicsObject(ScalableGraphicsObject *parent);

    QRectF boundingRect() const override = 0;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override = 0;

    virtual bool isClickable();
    virtual void setClickable(bool clickable); // TODO: split clickablehoverable into two independent functions
    virtual void setClickableHoverable(bool clickable, bool hoverable);
    virtual void setResetHoverOnClick(bool do_reset);

    virtual bool isSelected();
    virtual void setSelected(bool selected);
    virtual void setStickySelection(bool stickySelection);

signals:
    void clicked();
    void hoverEnter();
    void hoverLeave();
    void selectionChanged(bool selected);

protected slots:
    void onLanguageChanged();

protected:
    bool selected_;
    bool stickySelection_;
    bool clickable_;
    bool hoverable_;
    bool resetHoverOnClick_;

    bool pressed_;
    bool hovered_;

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

};
