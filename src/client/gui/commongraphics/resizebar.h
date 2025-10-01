#pragma once

#include "commongraphics/scalablegraphicsobject.h"

namespace CommonGraphics {

class ResizeBar : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ResizeBar(ScalableGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void resizeStarted();
    void resizeChange(int y);
    void resizeFinished();

private:
    bool bDragPressed_;
    QPoint dragPressPt_;
};

} // namespace CommonGraphics
