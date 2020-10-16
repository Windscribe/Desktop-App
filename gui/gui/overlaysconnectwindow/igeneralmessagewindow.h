#ifndef IGENERALMESSAGEWINDOW_H
#define IGENERALMESSAGEWINDOW_H

#include <QGraphicsObject>

class IGeneralMessageWindow
{
public:
    virtual ~IGeneralMessageWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void setTitle(QString title) = 0;
    virtual void setDescription(QString description) = 0;

    virtual void setErrorMode(bool error) = 0;
    virtual void updateScaling() = 0;

signals:
    virtual void acceptClick() = 0;

};

Q_DECLARE_INTERFACE(IGeneralMessageWindow, "IGeneralMessageWindow")


#endif // IGENERALMESSAGEWINDOW_H
