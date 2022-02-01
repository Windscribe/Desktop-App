#ifndef INEWSFEEDWINDOW_H
#define INEWSFEEDWINDOW_H

#include <QGraphicsObject>
#include <QSet>
#include <QString>

class INewsFeedWindow
{
public:
    virtual ~INewsFeedWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;
    virtual void setClickable(bool isClickable) = 0;
    virtual void updateScaling() = 0;

public slots:
    virtual void setMessages(const ProtoTypes::ArrayApiNotification &arr,
                             const QSet<qint64> &shownIds) = 0;
    virtual void setMessagesWithCurrentOverride(const ProtoTypes::ArrayApiNotification &arr,
                                                const QSet<qint64> &shownIds,
                                                int overrideCurrentMessageId) = 0;

signals:
    virtual void messageReaded(qint64 messageId) = 0;
    virtual void escClick() = 0;

};

Q_DECLARE_INTERFACE(INewsFeedWindow, "INewsFeedWindow")

#endif // INEWSFEEDWINDOW_H
