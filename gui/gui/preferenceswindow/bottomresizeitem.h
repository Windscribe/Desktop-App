#ifndef BOTTOMRESIZEITEM_H
#define BOTTOMRESIZEITEM_H

#include "CommonGraphics/scalablegraphicsobject.h"

namespace PreferencesWindow {

class BottomResizeItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit BottomResizeItem(ScalableGraphicsObject *parent = nullptr);
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

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
