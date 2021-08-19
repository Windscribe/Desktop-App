#ifndef IUPGRADEWINDOW_H
#define IUPGRADEWINDOW_H

#include <QGraphicsObject>

class IUpgradeWindow
{
public:
    virtual ~IUpgradeWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;
    virtual void updateScaling() = 0;

signals:
    virtual void acceptClick() = 0;
    virtual void cancelClick() = 0;
};

Q_DECLARE_INTERFACE(IUpgradeWindow, "IUpgradeWindow")


#endif // IUPGRADEWINDOW_H
