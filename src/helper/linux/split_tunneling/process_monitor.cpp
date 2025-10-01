#include "process_monitor.h"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <linux/cn_proc.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "cgroups.h"
#include "../utils.h"

#define SEND_MESSAGE_LEN (NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(enum proc_cn_mcast_op)))
#define RECV_MESSAGE_LEN (NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(struct proc_event)))
#define SEND_MESSAGE_SIZE (NLMSG_SPACE(SEND_MESSAGE_LEN))
#define RECV_MESSAGE_SIZE (NLMSG_SPACE(RECV_MESSAGE_LEN))
#define BUFSIZE (std::max(std::max(SEND_MESSAGE_SIZE, RECV_MESSAGE_SIZE), 1024UL))

void ProcessMonitor::monitorWorker(void *ctx)
{
    int ret;
    char buff[BUFSIZE];
    struct nlmsghdr *nlh = (struct nlmsghdr*)buff;

    spdlog::debug("process monitor thread started");
    running_ = true;

    while (running_) {
        struct pollfd pfd;
        pfd.fd = sock_;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, 250);
        if (ret < 0) {
            spdlog::error("process monitor poll error {}", ret);
            return;
        } else if (ret == 0) {
            continue;
        }

        ret = read(sock_, &buff, sizeof(buff));
        if (ret <= 0) {
            continue;
        }

        if (testing_) {
            break;
        }

        while (NLMSG_OK(nlh, ret)) {
            std::string cmd;

            if (nlh->nlmsg_type == NLMSG_NOOP) {
                NLMSG_NEXT(nlh, ret);
                continue;
            }
            if ((nlh->nlmsg_type == NLMSG_ERROR) || (nlh->nlmsg_type == NLMSG_OVERRUN)) {
                break;
            }
            struct cn_msg *cn_hdr = (struct cn_msg *)NLMSG_DATA(nlh);
            struct proc_event *ev = (struct proc_event *)cn_hdr->data;

            switch (ev->what) {
                case 0x00000001: // PROC_EVENT_FORK:
                    if (compareCmd(ev->event_data.fork.child_pid, apps_)) {
                        CGroups::instance().addApp(ev->event_data.fork.child_pid);
                    }
                    break;
                case 0x00000002: // PROC_EVENT_EXEC:
                    if (compareCmd(ev->event_data.exec.process_pid, apps_)) {
                        CGroups::instance().addApp(ev->event_data.exec.process_pid);
                    }
                    break;
                case 0x80000000: // PROC_EVENT_EXIT:
                    if (compareCmd(ev->event_data.exit.process_pid, apps_)) {
                        CGroups::instance().removeApp(ev->event_data.exit.process_pid);
                    }
                    break;
                default:
                    break;
            }

            if (nlh->nlmsg_type == NLMSG_DONE) {
                break;
            }
            nlh = NLMSG_NEXT(nlh, ret);
        }
    }
    running_ = false;
    close(sock_);
    sock_ = -1;
    spdlog::debug("process monitor thread exiting");
}

ProcessMonitor::ProcessMonitor() : isEnabled_(false), thread_(nullptr), sock_(-1), running_(false), functional_(false), testing_(false)
{
    selfTest();
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
    // Busy, try again later.  Not likely to happen since the helper should start much earlier than the app.
    if (testing_) {
        return false;
    }

    // Tested were run and found that we're not able to get some kernel events.  Fail here.
    if (!functional_) {
        spdlog::error("process monitor not functional");
        return false;
    }

    if (isEnabled_) {
        return true;
    }

    spdlog::debug("process monitor enable");

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

    spdlog::debug("process monitor disable");

    stopMonitoring();

    isEnabled_ = false;
}

void ProcessMonitor::addApp(const std::string &exe) {
    spdlog::info("process monitor add app: {}", exe);
    std::vector<pid_t> pids = findPids(exe);
    for (auto pid : pids) {
        CGroups::instance().addApp(pid);
    }
}

void ProcessMonitor::removeApp(const std::string &exe) {
    spdlog::info("process monitor remove app: {}", exe);
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
        spdlog::error("process monitor could not open /proc filesystem");
        return pids;
    }

    while ((ep = readdir(dp))) {
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
        spdlog::error("Failed to prepare monitoring");
        return false;
    }

    try {
        thread_ = new std::thread(&ProcessMonitor::monitorWorker, this, &sock_);
    } catch (std::exception &e) {
        spdlog::error("process monitor caught exception starting thread");
    }

    return true;
}

bool ProcessMonitor::prepareMonitoring()
{
    int ret = 0;
    struct sockaddr_nl addr;
    char buf[BUFSIZE];

    sock_ = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (sock_ == -1) {
        spdlog::error("Could not open netlink socket");
        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = CN_IDX_PROC;
    addr.nl_pid = getpid();

    ret = bind(sock_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        spdlog::error("Could not bind netlink socket");
        close(sock_);
        sock_ = -1;
        return false;
    }

    // Request for events
    struct nlmsghdr *nl_hdr = (struct nlmsghdr *)buf;
    struct cn_msg *cn_hdr = (struct cn_msg *)NLMSG_DATA(nl_hdr);
    enum proc_cn_mcast_op *mcop_msg = (enum proc_cn_mcast_op *)&cn_hdr->data[0];

    memset(buf, 0, sizeof(buf));
    *mcop_msg = PROC_CN_MCAST_LISTEN;

    nl_hdr->nlmsg_len = SEND_MESSAGE_LEN;
    nl_hdr->nlmsg_pid = getpid();
    nl_hdr->nlmsg_type = NLMSG_DONE;
    nl_hdr->nlmsg_flags = 0;
    nl_hdr->nlmsg_seq = 0;

    cn_hdr->id.idx = CN_IDX_PROC;
    cn_hdr->id.val = CN_VAL_PROC;
    cn_hdr->len = sizeof(enum proc_cn_mcast_op);
    cn_hdr->seq = 0;
    cn_hdr->ack = 0;

    ret = send(sock_, nl_hdr, nl_hdr->nlmsg_len, 0); // NOLINT
    if (ret == -1) {
        spdlog::error("Could not request events");
        close(sock_);
        sock_ = -1;
        return false;
    }
    return true;
}

void ProcessMonitor::stopMonitoring()
{
    if (sock_ != -1) {
        running_ = false;
    }

    if (thread_) {
        if (thread_->joinable()) {
            thread_->join();
        }
        delete thread_;
        thread_ = nullptr;
    }
}

void ProcessMonitor::selfTest()
{
    testing_ = true;

    if (!startMonitoring()) {
        functional_ = false;
        return;
    }

    // Starts a process to test the process monitor
    int ret = system("echo");
    UNUSED(ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Thread should no longer be running
    functional_ = !running_;

    // Cleanup
    stopMonitoring();

    spdlog::debug("process monitor self-test {}", (functional_ ? "successful" : "unsuccessful"));
    testing_ = false;
}
