#pragma once

#include "scalablegraphicsobject.h"
#include "../utils/imagewithshadow.h"

class ImageItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:

    explicit ImageItem(ScalableGraphicsObject * parent, const QString &imagePath, const QString &shadowImagePath = QString());

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

private:
    QString imagePath_;
    QString shadowImagePath_;
    QScopedPointer<ImageWithShadow> imageWithShadow_;
    int width_;
    int height_;
};
