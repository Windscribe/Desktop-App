#ifndef NODESSPEEDRATINGS_H
#define NODESSPEEDRATINGS_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QMutex>

// class thread safe access
class NodesSpeedRatings : public QObject
{
    Q_OBJECT
public:
    NodesSpeedRatings();
    virtual ~NodesSpeedRatings();

    bool getRatingForNode(const QString &hostname, int &rating);
    void thumbUpRatingForNode(const QString &hostname);
    void thumbDownRatingForNode(const QString &hostname);
    void clear();
    bool isEmpty();

signals:
    void nodeSpeedRatingChanged(const QString &hostname);

private:
    struct SpeedRatingNode
    {
        QString hostname;
        int     rating;
    };

    QVector<SpeedRatingNode *> ratings_;
    QMap<QString, SpeedRatingNode *> map_;
    QMutex mutex_;

    void readFromSettings();
    void writeToSettings();
};

#endif // NODESSPEEDRATINGS_H
