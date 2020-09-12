#ifndef APIINFO_NOTIFICATION_H
#define APIINFO_NOTIFICATION_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "ipc/generated_proto/types.pb.h"
#include "node.h"

namespace ApiInfo {

class NotificationData : public QSharedData
{
public:
    NotificationData() : isInitialized_(false),
                            id_(0),
                            date_(0),
                            perm_free_(0),
                            perm_pro_(0),
                            popup_(0) {}

    NotificationData(const NotificationData &other)
        : QSharedData(other),
          isInitialized_(other.isInitialized_),
          id_(other.id_),
          title_(other.title_),
          message_(other.message_),
          date_(other.date_),
          perm_free_(other.perm_free_),
          perm_pro_(other.perm_pro_),
          popup_(other.popup_) {}

    ~NotificationData() {}

    bool isInitialized_;
    qint64 id_;
    QString title_;
    QString message_;
    qint64 date_;
    int perm_free_;
    int perm_pro_;
    int popup_;
};

// implicitly shared class Notification
class Notification
{
public:
    explicit Notification() { d = new NotificationData; }
    Notification(const Notification &other) : d (other.d) { }

    bool initFromJson(const QJsonObject &json);
    ProtoTypes::ApiNotification getProtoBuf() const;

private:
    QSharedDataPointer<NotificationData> d;
};

} //namespace ApiInfo

#endif // APIINFO_NOTIFICATION_H
