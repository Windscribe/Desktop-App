#ifndef LOADINGICONITEM_H
#define LOADINGICONITEM_H

#include <QMovie>
#include "commongraphics/scalablegraphicsobject.h"

class LoadingIconItem: public ScalableGraphicsObject
{
public:
    explicit LoadingIconItem(ScalableGraphicsObject *parent, QString file, int width, int height);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void setOpacity(double opacity);
    void start();
    void stop();

private slots:
    void onFrameChanged();

private:
    QMovie *movie_;

    double opacity_;
    int width_;
    int height_;
};

#endif // LOADINGICONITEM_H
