#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <QIcon>



class IconManager
{
public:
    IconManager();

    const QIcon *getDisconnectedIcon() const { return &disconnectedIcon_; }
    const QIcon *getConnectingIcon() const { return &connectingIcon_; }
    const QIcon *getConnectedIcon() const { return &connectedIcon_; }

private:
    QIcon disconnectedIcon_;
    QIcon connectingIcon_;
    QIcon connectedIcon_;

};

#endif // ICONMANAGER_H
