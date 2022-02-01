#ifndef ILOGGINGINWINDOW_H
#define ILOGGINGINWINDOW_H

#include <QString>
#include <QGraphicsObject>

// abstract interface for logging you in window
class ILoggingInWindow
{
public:
    virtual ~ILoggingInWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void startAnimation() = 0;
    virtual void stopAnimation() = 0;

    virtual void setMessage(const QString &msg) = 0;
    virtual void setAdditionalMessage(const QString &msg) = 0;

    virtual void updateScaling() = 0;
};

Q_DECLARE_INTERFACE(ILoggingInWindow, "ILoggingInWindow")

#endif // ILOGGINGINWINDOW_H
