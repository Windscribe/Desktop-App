#ifndef IINITWINDOW_H
#define IINITWINDOW_H

#include <QGraphicsObject>

class IInitWindow
{
public:
    virtual ~IInitWindow() {}
    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void resetState() = 0;
    virtual void startSlideAnimation() = 0;
    virtual void startWaitingAnimation() = 0;

    virtual void setClickable(bool clickable) = 0;
    virtual void setAdditionalMessage(const QString &msg, bool useSmallFont) = 0;
    virtual void setCropHeight(int height) = 0;
    virtual void setHeight(int height) = 0;
    virtual void setCloseButtonVisible(bool visible) = 0;

    virtual void updateScaling() = 0;

signals:
    virtual void abortClicked() = 0;

};

Q_DECLARE_INTERFACE(IInitWindow, "IInitWindow")

#endif // IINITWINDOW_H
