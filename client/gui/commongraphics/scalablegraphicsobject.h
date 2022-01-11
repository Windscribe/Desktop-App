#ifndef SCALABLEGRAPHICSOBJECT_H
#define SCALABLEGRAPHICSOBJECT_H

#include <QGraphicsObject>

class ScalableGraphicsObject : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit ScalableGraphicsObject(QGraphicsObject *parent = nullptr);

    virtual void updateScaling();

};

#endif // SCALABLEGRAPHICSOBJECT_H
