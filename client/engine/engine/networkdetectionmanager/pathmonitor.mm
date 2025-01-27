#include "pathmonitor.h"
#include "utils/log/categories.h"

PathMonitor::PathMonitor(): curIsOnline_(true)
{
  monitor_ = nw_path_monitor_create();
  nw_path_monitor_set_queue(monitor_, dispatch_get_main_queue());

  nw_path_monitor_set_update_handler(monitor_, ^(nw_path_t _Nonnull path) {
        bool isOnline = false;
        nw_path_status_t status = nw_path_get_status(path);
        switch (status) {
            case nw_path_status_invalid: {
                qCInfo(LOG_BASIC) << "PathMonitor: network path is invalid";
                break;
            }
            case nw_path_status_satisfied: {
                qCInfo(LOG_BASIC) << "PathMonitor: network is usable";
                isOnline = true;
                break;
            }
            case nw_path_status_satisfiable: {
                qCInfo(LOG_BASIC) << "PathMonitor: network may be usable";
                isOnline = true;
                break;
            }
            case nw_path_status_unsatisfied: {
                qCInfo(LOG_BASIC) << "PathMonitor: network is not usable";
                break;
            }
        }
        if (isOnline != curIsOnline_) {
          curIsOnline_ = isOnline;
          emit connectivityStateChanged(curIsOnline_);
        }
    });
  nw_path_monitor_start(monitor_);
  int g = 0;
}

PathMonitor::~PathMonitor()
{
  nw_path_monitor_cancel(monitor_);
  nw_release(monitor_);
}
