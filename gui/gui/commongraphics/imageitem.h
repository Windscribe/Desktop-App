#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include "commongraphics/scalablegraphicsobject.h"

class ImageItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:

    explicit ImageItem(const QString &imagePath, ScalableGraphicsObject * parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

private:
    QString imagePath_;
    int width_;
    int height_;
};


#endif // IMAGEITEM_H
