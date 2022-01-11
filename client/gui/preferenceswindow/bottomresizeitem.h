#ifndef BOTTOMRESIZEITEM_H
#define BOTTOMRESIZEITEM_H

#include "commongraphics/scalablegraphicsobject.h"

namespace PreferencesWindow {

class BottomResizeItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit BottomResizeItem(ScalableGraphicsObject *parent = nullptr);
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

} // namespace PreferencesWindow

#endif // BOTTOMRESIZEITEM_H
