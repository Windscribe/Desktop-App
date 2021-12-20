#include "nodeselectionalgorithm.h"
#include "utils/utils.h"

namespace locationsmodel {

int NodeSelectionAlgorithm::selectRandomNodeBasedOnWeight(const QVector<QSharedPointer<const BaseNode> > &nodes)
{
    if (nodes.count() == 1)
    {
        return 0;
    }
    else if (nodes.count() == 0)
    {
        return -1;
    }
    else
    {
        QVector<double> weights;
        weights.reserve(nodes.size());
        double sum_weights = 0;
        for (int i = 0; i < nodes.size(); ++i)
        {
            double w = nodes[i]->getWeight();
            weights << w;
            sum_weights += w;
        }

        QVector<double> p;
        for (int i = 0; i < nodes.size(); ++i)
        {
            p << weights[i] / sum_weights;
        }
        return getRandomEvent(p);
    }
}

int NodeSelectionAlgorithm::getRandomEvent(QVector<double> &p)
{
    Q_ASSERT(p.size() > 0);
    const double eps = 1e-9;

    double r = Utils::generateDoubleRandom(0.0, 1.0); // generates a random number distribute in [0,1];
    for (int i = 0; i < p.size(); ++i)
    {
        r -= p[i];
        if (r < eps)
        {
            return i;
        }
    }
    Q_ASSERT(false);
    return 0;
}

} //namespace locationsmodel
