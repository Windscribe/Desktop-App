#ifndef IGENERALMESSAGETWOBUTTONWINDOW_H
#define IGENERALMESSAGETWOBUTTONWINDOW_H

#include <QGraphicsObject>

class IGeneralMessageTwoButtonWindow
{
public:
    virtual ~IGeneralMessageTwoButtonWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void setTitle(const QString title) = 0;
    virtual void setIcon(const QString icon) = 0;

    virtual void setAcceptText(const QString text) = 0;
    virtual void setRejectText(const QString text) = 0;

    virtual void setBackgroundShapedToConnectWindow(bool shapedToConnectWindow) = 0;

    virtual void setShutdownAnimationMode(bool isShutdownAnimationMode) = 0;

    virtual void clearSelection() = 0;

    virtual void updateScaling() = 0;

    enum Selection { NONE, ACCEPT, REJECT };
    virtual void changeSelection(Selection selection) = 0;

signals:
    virtual void acceptClick() = 0;
    virtual void rejectClick() = 0;

};

Q_DECLARE_INTERFACE(IGeneralMessageTwoButtonWindow, "IGeneralMessageTwoButtonWindow")


#endif // IGENERALMESSAGETWOBUTTONWINDOW_H
