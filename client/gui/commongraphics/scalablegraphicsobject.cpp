#include "scalablegraphicsobject.h"

ScalableGraphicsObject::ScalableGraphicsObject(QGraphicsObject *parent) : QGraphicsObject(parent)
{

}

void ScalableGraphicsObject::updateScaling()
{
    prepareGeometryChange();

    // update scaling for all childs
    const auto child_items = childItems();
    for (QGraphicsItem *it : child_items)
    {
        ScalableGraphicsObject *o = dynamic_cast<ScalableGraphicsObject*>(it);
        if (o)
        {
            o->updateScaling();
        }
    }
}
