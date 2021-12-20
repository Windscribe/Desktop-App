#ifndef IEMERGENCYCONNECTWINDOW_H
#define IEMERGENCYCONNECTWINDOW_H

#include <QGraphicsObject>
#include "../backend/types/types.h"

class IEmergencyConnectWindow
{
public:
    virtual ~IEmergencyConnectWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void setState(ProtoTypes::ConnectState state) = 0;

    virtual void setClickable(bool isClickable) = 0;

    virtual void updateScaling() = 0;
signals:
    virtual void minimizeClick() = 0;
    virtual void closeClick() = 0;
    virtual void escapeClick() = 0;

    virtual void connectClick() = 0;
    virtual void disconnectClick() = 0;

    virtual void windscribeLinkClick() = 0;

};

Q_DECLARE_INTERFACE(IEmergencyConnectWindow, "IEmergencyConnectWindow")

#endif // IEMERGENCYCONNECTWINDOW_H
