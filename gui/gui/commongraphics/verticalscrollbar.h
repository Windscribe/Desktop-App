#ifndef VERTICALSCROLLBAR_H
#define VERTICALSCROLLBAR_H

#include "commongraphics/scalablegraphicsobject.h"
#include <QVariantAnimation>

class VerticalScrollBar : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    static constexpr int SCROLL_WIDTH = 3; // clickable width

    explicit VerticalScrollBar(int height, double barPortion, ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setHeight(int height, double barPortion);

    void moveBarToPercentPos(double posPercentY);

signals:
   void clicked();
   void moved(double posPercentY);

private slots:
   void onBarPosYChanged(const QVariant &value);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

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
