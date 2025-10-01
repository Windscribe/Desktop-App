#pragma once

#include "locationnode.h"

namespace locationsmodel {

class NodeSelectionAlgorithm
{
public:
    static int selectRandomNodeBasedOnWeight(const QVector< QSharedPointer<const BaseNode> > &nodes);

private:
    static int getRandomEvent(QVector<double> &p);
};

} //namespace locationsmodel
