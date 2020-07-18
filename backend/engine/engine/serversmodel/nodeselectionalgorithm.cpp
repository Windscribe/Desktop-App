#include "nodeselectionalgorithm.h"
#include <QEasingCurve>
#include "utils/utils.h"

const double NodeSelectionAlgorithm::WEIGHT_NODE = 0.25;
const double NodeSelectionAlgorithm::WEIGHT_RATING = 0.25;
const double NodeSelectionAlgorithm::WEIGHT_LATENCY = 0.5;

int NodeSelectionAlgorithm::selectRandomNodeBasedOnWeight(const QVector<ServerNode> &nodes, NodesSpeedRatings *nodesSpeedRating, QVector<double> &p)
{
    p.clear();

    if (nodes.count() == 1)
    {
        p << 1.0;
        return 0;
    }
    else if (nodes.count() == 0)
    {
        return -1;
    }
    else
    {
        //adjust weights with user speed ratings
        QVector<double> adjustedWeights;
        adjustedWeights.reserve(nodes.size());
        double sum_weights = 0;
        for (int i = 0; i < nodes.size(); ++i)
        {
            double w = nodes[i].getWeight();
            int rating;
            if (nodesSpeedRating->getRatingForNode(nodes[i].getHostname(), rating))
            {
                if (rating > 0)
                {
                    w *= (rating+1);
                }
                else if (rating < 0)
                {
                    w /= ((-rating)+1);
                }
            }

            adjustedWeights << w;
            sum_weights += w;
        }


        for (int i = 0; i < nodes.size(); ++i)
        {
            p << adjustedWeights[i] / sum_weights;
        }
        return getRandomEvent(p);
    }
}

int NodeSelectionAlgorithm::selectRandomNodeBasedOnLatency(QVector<NodeInfo> &nodes, QVector<double> &p)
{
    p.clear();
    if (nodes.count() == 1)
    {
        if (nodes[0].pingTime != -1)
        {
            p << 1.0;
            return 0;
        }
        else
        {
            p << 0.0;
            return -1;

        }
    }
    else if (nodes.count() == 0)
    {
        return -1;
    }
    else
    {
        // calculate probabilities based on latency
        double sumLatency = 0;
        int maxPingTime = 0;
        int cntNodesWithLatency = 0;
        for (int i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].pingTime != -1)
            {
                sumLatency += nodes[i].pingTime;
                cntNodesWithLatency++;
                if (nodes[i].pingTime > maxPingTime)
                {
                    maxPingTime = nodes[i].pingTime;
                }
            }
        }

        if (cntNodesWithLatency == 0)
        {
            for (int i = 0; i < nodes.size(); ++i)
            {
                p << 0.0;
            }
            return -1;
        }

        if (maxPingTime <= 1000)
        {
            maxPingTime = 1000;
        }

        double averageLatency = sumLatency / (double)cntNodesWithLatency;

        QVector<double> probabilitiesBasedOnLatency;
        probabilitiesBasedOnLatency.reserve(nodes.size());
        double sum_weights = 0;
        for (int i = 0; i < nodes.size(); ++i)
        {
            double w = 0;
            if (nodes[i].pingTime != -1 && nodes[i].pingTime <= averageLatency*2)
            {
                QEasingCurve curve(QEasingCurve::InQuart);
                w = curve.valueForProgress(1.0 - (double)nodes[i].pingTime / (double)maxPingTime);
                nodes[i].isNodeSkipped = false;
            }
            else
            {
                nodes[i].isNodeSkipped = true;
            }

            probabilitiesBasedOnLatency << w;
            sum_weights += w;
        }

        for (int i = 0; i < nodes.size(); ++i)
        {
            if (sum_weights > 0)
            {
                probabilitiesBasedOnLatency[i] = probabilitiesBasedOnLatency[i] / sum_weights;
            }
            else
            {
                probabilitiesBasedOnLatency[i] = 0;
            }
        }

        // calculate probabilities based on nodes weights
        QVector<double> probabilitiesBasedOnWeight;
        probabilitiesBasedOnWeight.reserve(nodes.size());

        sum_weights = 0;
        for (int i = 0; i < nodes.size(); ++i)
        {
            if (!nodes[i].isNodeSkipped)
            {
                sum_weights += nodes[i].weight;
            }
        }
        for (int i = 0; i < nodes.size(); ++i)
        {
            if (!nodes[i].isNodeSkipped)
            {
                probabilitiesBasedOnWeight << nodes[i].weight / sum_weights;
            }
            else
            {
                probabilitiesBasedOnWeight << 0.0;
            }
        }

        // calculate probabilities based on user rating
        QVector<double> probabilitiesBasedOnRating;
        probabilitiesBasedOnRating.reserve(nodes.size());

        sum_weights = 0;
        for (int i = 0; i < nodes.size(); ++i)
        {
            if (!nodes[i].isNodeSkipped)
            {
                double w = 1;
                if (nodes[i].userRating > 0)
                {
                    w *= (nodes[i].userRating+1);
                }
                else if (nodes[i].userRating < 0)
                {
                    w /= ((-nodes[i].userRating)+1);
                }

                sum_weights += w;
                probabilitiesBasedOnRating << w;
            }
            else
            {
                probabilitiesBasedOnRating << 0.0;
            }
        }
        for (int i = 0; i < nodes.size(); ++i)
        {
            probabilitiesBasedOnRating[i] = probabilitiesBasedOnRating[i] / sum_weights;
        }

        // calculate final probabilities
        QVector<double> probabilities;
        probabilities.reserve(nodes.size());
        sum_weights = 0;
        for (int i = 0; i < nodes.size(); ++i)
        {
            double w = probabilitiesBasedOnWeight[i]*WEIGHT_NODE + probabilitiesBasedOnRating[i]*WEIGHT_RATING + probabilitiesBasedOnLatency[i]*WEIGHT_LATENCY;
            probabilities << w;
            sum_weights += w;

        }

        for (int i = 0; i < nodes.size(); ++i)
        {
            probabilities[i] = probabilities[i] / sum_weights;
        }

        p = probabilities;
        return getRandomEvent(probabilities);
    }

    return 0;
}

void NodeSelectionAlgorithm::autoTest()
{
    /*QVector<NodeInfo> nodes;
    QVector<double> p;

    NodeInfo ni;

    ni.pingTime = 10;
    ni.userRating = 0;
    ni.weight = 1;
    nodes << ni;

    ni.pingTime = 25;
    ni.userRating = 0;
    ni.weight = 16;
    nodes << ni;

    ni.pingTime = -1;
    ni.userRating = 0;
    ni.weight = 1;
    nodes << ni;

    ni.pingTime = 40;
    ni.userRating = 0;
    ni.weight = 1;
    nodes << ni;

    ni.pingTime = -1;
    ni.userRating = 0;
    ni.weight = 1;
    nodes << ni;


    int sel = selectRandomNodeBasedOnLatency(nodes, p);

    int g = 0;*/
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
