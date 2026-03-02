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
    void startOfflineDebounce();
    void cancelOfflineDebounce();

    nw_path_monitor_t monitor_;
    dispatch_source_t debounceTimer_;
    bool curIsOnline_;
};
