#ifndef IEXTERNALCONFIGWINDOW_H
#define IEXTERNALCONFIGWINDOW_H

#include <QGraphicsObject>

class IExternalConfigWindow
{
public:
    virtual ~IExternalConfigWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void setClickable(bool isClickable) = 0;
    virtual void updateScaling() = 0;

signals:
    virtual void buttonClick() = 0;
    virtual void escapeClick() = 0;

    virtual void minimizeClick() = 0;
    virtual void closeClick() = 0;

};

Q_DECLARE_INTERFACE(IExternalConfigWindow, "IExternalConfigWindow")


#endif // IEXTERNALCONFIGWINDOW_H
