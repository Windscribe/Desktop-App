#ifndef VERTICALSCROLLBAR_H
#define VERTICALSCROLLBAR_H

#include "commongraphics/scalablegraphicsobject.h"
#include <QVariantAnimation>

class VerticalScrollBar : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    static const int SCROLL_WIDTH = 3; // clickable width

    explicit VerticalScrollBar(int height, double barPortion, ScalableGraphicsObject *parent);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setHeight(int height, double barPortion);

    void moveBarToPercentPos(double posPercentY);

signals:
   void clicked();
   void moved(double posPercentY);

private slots:
   void onBarPosYChanged(const QVariant &value);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:

    int height_;

    int drawWidth_; // visible width
    int drawWidthPosY_;

    int curBarPosY_;
    int curBarHeight_;

    bool inBarRegion(int y);

    bool mouseHold_;
    int mouseOnClickY_;

    QVariantAnimation barPosYAnimation_;

};

#endif // VERTICALSCROLLBAR_H
