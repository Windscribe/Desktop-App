#include "nodesspeedratings.h"
#include <QDataStream>
#include <QSettings>

NodesSpeedRatings::NodesSpeedRatings() : mutex_(QMutex::Recursive)
{
    readFromSettings();
}

NodesSpeedRatings::~NodesSpeedRatings()
{
    writeToSettings();
    clear();
}

bool NodesSpeedRatings::getRatingForNode(const QString &hostname, int &rating)
{
    QMutexLocker locker(&mutex_);

    auto it = map_.find(hostname);
    if (it != map_.end())
    {
        rating = (*it)->rating;
        return true;
    }
    else
    {
        return false;
    }
}

void NodesSpeedRatings::thumbUpRatingForNode(const QString &hostname)
{
    QMutexLocker locker(&mutex_);
    auto it = map_.find(hostname);
    if (it != map_.end())
    {
        (*it)->rating++;
    }
    else
    {
        SpeedRatingNode *n = new SpeedRatingNode();
        n->rating = 1;
        n->hostname = hostname;
        ratings_ << n;
        map_[n->hostname] = n;
    }
    emit nodeSpeedRatingChanged(hostname);
}

void NodesSpeedRatings::thumbDownRatingForNode(const QString &hostname)
{
    QMutexLocker locker(&mutex_);
    auto it = map_.find(hostname);
    if (it != map_.end())
    {
        (*it)->rating--;
    }
    else
    {
        SpeedRatingNode *n = new SpeedRatingNode();
        n->rating = -1;
        n->hostname = hostname;
        ratings_ << n;
        map_[n->hostname] = n;
    }
    emit nodeSpeedRatingChanged(hostname);
}

void NodesSpeedRatings::clear()
{
    QMutexLocker locker(&mutex_);
    Q_FOREACH(SpeedRatingNode *n, ratings_)
    {
        delete n;
    }
    ratings_.clear();
    map_.clear();
}

bool NodesSpeedRatings::isEmpty()
{
    QMutexLocker locker(&mutex_);
    return ratings_.isEmpty();
}

void NodesSpeedRatings::readFromSettings()
{
    QMutexLocker locker(&mutex_);
    QSettings settings;
    if (settings.contains("speedRatings"))
    {
        QByteArray arr = settings.value("speedRatings").toByteArray();
        int cnt;
        QDataStream stream(&arr, QIODevice::ReadOnly);
        stream >> cnt;
        for(int i = 0; i < cnt; ++i)
        {
            SpeedRatingNode *n = new SpeedRatingNode();
            stream >> n->hostname;
            stream >> n->rating;
            ratings_ << n;
            map_[n->hostname] = n;
        }
    }
}

void NodesSpeedRatings::writeToSettings()
{
    QMutexLocker locker(&mutex_);
    QByteArray arr;
    {
        QDataStream stream(&arr, QIODevice::WriteOnly);
        stream << ratings_.count();
        Q_FOREACH(SpeedRatingNode *n, ratings_)
        {
            stream << n->hostname;
            stream << n->rating;
        }
    }
    QSettings settings;
    settings.setValue("speedRatings", arr);
}
