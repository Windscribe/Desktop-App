#include "process_monitor.h"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>

#include "cgroups.h"
#include "../logger.h"
#include "../utils.h"

void ProcessMonitor::monitorWorker(void *ctx)
{
    int sock = *(int *)ctx;
    int ret;
    struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
        struct nlmsghdr nl_hdr;
        struct __attribute__ ((__packed__)) {
            struct cn_msg cn_msg;
            struct proc_event proc_ev;
        };
    } nlcn_msg;

    while (true) {
        ret = recv(sock, &nlcn_msg, sizeof(nlcn_msg), 0);
        if (ret <= 0) {
            return;
        }

        std::string cmd;
        switch (nlcn_msg.proc_ev.what) {
            case 1: // PROC_EVENT_FORK:
                if (compareCmd(nlcn_msg.proc_ev.event_data.fork.child_pid, apps_)) {
                    CGroups::instance().addApp(nlcn_msg.proc_ev.event_data.fork.child_pid);
                }
                break;
            case 2: // PROC_EVENT_EXEC:
                if (compareCmd(nlcn_msg.proc_ev.event_data.exec.process_pid, apps_)) {
                    CGroups::instance().addApp(nlcn_msg.proc_ev.event_data.exec.process_pid);
                }
                break;
            case 0: // PROC_EVENT_EXIT:
                if (compareCmd(nlcn_msg.proc_ev.event_data.exit.process_pid, apps_)) {
                    CGroups::instance().addApp(nlcn_msg.proc_ev.event_data.exit.process_pid);
                }
                break;
            default:
                break;
        }
    }
}

ProcessMonitor::ProcessMonitor() : isEnabled_(false), thread_(nullptr), sock_(-1)
{
}

ProcessMonitor::~ProcessMonitor()
{
    stopMonitoring();
}

void ProcessMonitor::setApps(const std::vector<std::string> &apps)
{
    if (isEnabled_) {
        for (auto app : apps) {
            if (std::find(apps_.begin(), apps_.end(), app) == apps_.end()) {
                addApp(app);
            }
        }

        for (auto app : apps_) {
            if (std::find(apps.begin(), apps.end(), app) == apps.end()) {
                removeApp(app);
            }
        }
    }

    apps_ = apps;
}

bool ProcessMonitor::enable()
{
    if (isEnabled_) {
        return true;
    }

    Logger::instance().out("process monitor enable");

    if (!startMonitoring()) {
        return false;
    }

    for (auto app : apps_) {
        addApp(app);
    }
    isEnabled_ = true;
    return true;
}

void ProcessMonitor::disable()
{
    if (!isEnabled_) {
        return;
    }

    Logger::instance().out("process monitor disable");

    stopMonitoring();

    isEnabled_ = false;
}

void ProcessMonitor::addApp(const std::string &exe) {
    Logger::instance().out("process monitor add app: %s", exe.c_str());
    std::vector<pid_t> pids = findPids(exe);
    for (auto pid : pids) {
        CGroups::instance().addApp(pid);
    }
}

void ProcessMonitor::removeApp(const std::string &exe) {
    Logger::instance().out("process monitor remove app: %s", exe.c_str());
    std::vector<pid_t> pids = findPids(exe);
    for (auto pid : pids) {
        CGroups::instance().removeApp(pid);
    }
}

std::vector<pid_t> ProcessMonitor::findPids(const std::string &exe)
{
    std::vector<pid_t> pids;

    DIR *dp = NULL;
    struct dirent *ep;

    dp = opendir("/proc");
    if (dp == NULL) {
        Logger::instance().out("process monitor could not open /proc filesystem");
        return pids;
    }

    while (ep = readdir(dp)) {
        // numeric directories are pids in /proc
        if (ep->d_type == DT_DIR && ep->d_name[0] >= '0' && ep->d_name[0] <= '9') {
            if (compareCmd(std::stoi(ep->d_name), {exe})) {
                pids.push_back(std::stoi(ep->d_name));
            }
        }
    }
    closedir(dp);

    return pids;
}

bool ProcessMonitor::compareCmd(pid_t pid, const std::vector<std::string> &exes) {
    std::string cmd = getCmdByPid(pid);

    for (auto exe : exes) {
        if (cmd == exe) {
            return true;
        }

        // handle snap
        int idx = exe.find("/snap/");
        if (idx != std::string::npos) {
            std::string prefix = exe.substr(0, idx + 6);
            std::string suffix = exe.substr(exe.rfind("/"));
            if (cmd.rfind(prefix, 0) == 0 && cmd.find(suffix, cmd.size() - suffix.length()) == cmd.size() - suffix.length()) {
                return true;
            }
        }

        // handle flatpak
        if (cmd.rfind("/app/", 0) == 0 && exe.rfind("/app/", 0) == 0) {
            std::string suffix = exe.substr(exe.rfind("/"));
            if (cmd.find(suffix, cmd.size() - suffix.length()) == cmd.size() - suffix.length()) {
                return true;
            }
        }
    }

    return false;
}

std::string ProcessMonitor::getCmdByPid(pid_t pid)
{
    char buf[PATH_MAX];

    memset(buf, 0, PATH_MAX);

    int ret = readlink((std::string("/proc/") + std::to_string(pid) + "/exe").c_str(), buf, PATH_MAX - 1);
    if (ret < 0) {
        return std::string();
    }
    return buf;
}

bool ProcessMonitor::startMonitoring()
{
    if (thread_) {
        // already monitoring, do nothing
        return true;
    }

    if (!prepareMonitoring()) {
        Logger::instance().out("Failed to prepare monitoring");
        return false;
    }

    thread_ = new std::thread(&ProcessMonitor::monitorWorker, this, &sock_);
    return true;
}

bool ProcessMonitor::prepareMonitoring()
{
    int ret = 0;
    struct sockaddr_nl addr;

    sock_ = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (sock_ == -1) {
        Logger::instance().out("Could not open netlink socket");
        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = CN_IDX_PROC;
    addr.nl_pid = getpid();

    ret = bind(sock_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        Logger::instance().out("Could not bind netlink socket");
        close(sock_);
        sock_ = -1;
        return false;
    }

    // Request for events
    struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
        struct nlmsghdr nl_hdr;
        struct __attribute__ ((__packed__)) {
            struct cn_msg cn_msg;
            enum proc_cn_mcast_op cn_mcast;
        };
    } nlcn_msg;

    memset(&nlcn_msg, 0, sizeof(nlcn_msg));
    nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
    nlcn_msg.nl_hdr.nlmsg_pid = getpid();
    nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;

    nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
    nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
    nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);
    nlcn_msg.cn_mcast = PROC_CN_MCAST_LISTEN;

    ret = send(sock_, &nlcn_msg, sizeof(nlcn_msg), 0);
    if (ret == -1) {
        Logger::instance().out("Could not request events");
        close(sock_);
        sock_ = -1;
        return false;
    }
    return true;
}

void ProcessMonitor::stopMonitoring()
{
    // closing the socket will cause the thread to exit
    close(sock_);
    sock_ = -1;

    if (thread_) {
        thread_->join();
        delete thread_;
        thread_ = nullptr;
    }
}


