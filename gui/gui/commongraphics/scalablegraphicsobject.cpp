#include "scalablegraphicsobject.h"

ScalableGraphicsObject::ScalableGraphicsObject(QGraphicsObject *parent) : QGraphicsObject(parent)
{

}

void ScalableGraphicsObject::updateScaling()
{
    prepareGeometryChange();

    // update scaling for all childs
    Q_FOREACH (QGraphicsItem *it, childItems())
    {
        ScalableGraphicsObject *o = dynamic_cast<ScalableGraphicsObject*>(it);
        if (o)
        {
            o->updateScaling();
        }
    }
}
