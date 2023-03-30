#pragma once

#include <QGraphicsObject>
#include <QSet>
#include <QString>
#include "types/notification.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/resizablewindow.h"

class INewsFeedWindow : public ResizableWindow
{
public:
    explicit INewsFeedWindow(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
        : ResizableWindow(parent, preferences, preferencesHelper) {}
    virtual ~INewsFeedWindow() {}

    virtual void updateRead() = 0;

public slots:
    virtual void setMessages(const QVector<types::Notification> &arr,
                             const QSet<qint64> &shownIds) = 0;
    virtual void setMessagesWithCurrentOverride(const QVector<types::Notification> &arr,
                                                const QSet<qint64> &shownIds,
                                                int overrideCurrentMessageId) = 0;

signals:
    virtual void messageRead(qint64 messageId) = 0;
};

Q_DECLARE_INTERFACE(INewsFeedWindow, "INewsFeedWindow")