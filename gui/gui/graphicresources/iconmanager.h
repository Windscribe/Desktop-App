#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <QIcon>



class IconManager
{
public:
    IconManager();

    QIcon &getDisconnectedIcon() { return disconnectedIcon_; }
    QIcon &getConnectingIcon() { return connectingIcon_; }
    QIcon &getConnectedIcon() { return connectedIcon_; }

private:
    QIcon disconnectedIcon_;
    QIcon connectingIcon_;
    QIcon connectedIcon_;

};

#endif // ICONMANAGER_H
