#pragma once

#include <QVariantAnimation>
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
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

signals:
    void resizeStarted();
    void resizeChange(int y);
    void resizeFinished();

private slots:
    void onOpacityChanged(const QVariant &value);

private:
    bool bDragPressed_;
    QPoint dragPressPt_;
    double iconOpacity_;
    QVariantAnimation opacityAnimation_;

    QRectF getIconRect() const;
};

} // namespace CommonGraphics
