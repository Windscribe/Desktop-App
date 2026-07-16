#pragma once

#include <sys/types.h>

#include <string>
#include <vector>

namespace OvpnMgmt {

void killOpenVPN();
bool mgmtClientUserForUid(uid_t uid, std::string &out);
bool waitAndRelaxPerms(uid_t clientUid, pid_t ovpnPid);

} // namespace OvpnMgmt
