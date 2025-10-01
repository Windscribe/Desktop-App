#pragma once

#include <QObject>
#import <Network/Network.h>

class PathMonitor : public QObject
{
    Q_OBJECT
public:

    PathMonitor();
    ~PathMonitor();

    bool isOnline() const { return curIsOnline_; }

signals:
    void connectivityStateChanged(bool isOnline);

private:
    nw_path_monitor_t monitor_;
    bool curIsOnline_;
};
