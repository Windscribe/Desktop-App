#ifndef VERTICALSCROLLBAR_H
#define VERTICALSCROLLBAR_H

#include "commongraphics/scalablegraphicsobject.h"
#include <QVariantAnimation>

class VerticalScrollBar : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit VerticalScrollBar(int height, double barPortion, ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setHeight(int height, double barPortion);
    void moveBarToPercentPos(double posPercentY);

    // due to weirdness at edge we keep a larger click region than what is actually drawn -- note difference between this and boundingRect geo
    int visibleWidth();

signals:
   void clicked();
   void moved(double posPercentY);
   void hoverEnter();
   void hoverLeave();

private slots:
   void onBarPosYChanged(const QVariant &value);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    static constexpr int SCROLL_WIDTH = 12; // clickable width

    int height_;

    int curBarPosY_;
    int curBarHeight_;

    bool inBarRegion(int y);

    bool mouseHold_;
    int mouseOnClickY_;

    QVariantAnimation barPosYAnimation_;

};

#endif // VERTICALSCROLLBAR_H
