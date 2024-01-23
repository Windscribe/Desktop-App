#pragma once

#include <string>
#include "../../../posix_common/helper_commands.h"

class CGroups
{
public:
    static CGroups& instance()
    {
        static CGroups cg;
        return cg;
    }

    bool enable(CMD_SEND_CONNECT_STATUS &connectStatus, bool isAllowLanTraffic, bool isExclude);
    void disable();

    void addApp(pid_t pid);
    void removeApp(pid_t pid);

    std::string mark() const { return mark_; };
    std::string netClassId() const { return netClassId_; };

private:
    const std::string mark_ = "0xdecafbad";
    const std::string netClassId_ = "0xcafecafe";

    std::string net_cls_root_;

    CGroups();
    ~CGroups();

    std::string findNetclsRoot();
};
