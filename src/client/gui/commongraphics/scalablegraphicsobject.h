#pragma once

#include <QGraphicsObject>

class ScalableGraphicsObject : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit ScalableGraphicsObject(QGraphicsObject *parent = nullptr);

    virtual void updateScaling();
};
