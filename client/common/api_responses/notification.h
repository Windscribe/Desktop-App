#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>

namespace api_responses {

struct Notification
{
    Notification() {}
    Notification(const QJsonObject &json);

    bool operator==(const Notification &other) const;
    bool operator!=(const Notification &other) const;

    friend QDataStream& operator <<(QDataStream &stream, const Notification &o);
    friend QDataStream& operator >>(QDataStream &stream, Notification &o);

    qint64 id = 0;
    QString title;
    QString message;
    qint64 date = 0;
    int permFree = 0;
    int permPro = 0;
    int popup = 0;

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

class Notifications
{
public:
    Notifications(const std::string &json);

    QVector<Notification> notifications() const { return notifications_; }

private:
    QVector<Notification> notifications_;
};

} //namespace api_responses
