#ifndef ICONHOVERWITHTEXTBUTTON_H
#define ICONHOVERWITHTEXTBUTTON_H

#include <QGraphicsObject>
#include <CommonGraphics/iconhoverbutton.h>

class IconHoverWithTextButton : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit IconHoverWithTextButton(int width, int height, const QString &imagePath, QGraphicsObject * parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

signals:
   void clicked();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
   IconHoverButton *button_;

   QString text_;

   int width_;
   int height_;

};

#endif // ICONHOVERWITHTEXTBUTTON_H
