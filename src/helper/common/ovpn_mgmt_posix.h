#pragma once

#include <sys/types.h>

#include <string>
#include <vector>

namespace OvpnMgmt {

void killOpenVPN();
bool mgmtClientUserForUid(uid_t uid, std::string &out);
// outDetail receives the daemon's fatal text when OpenVPN exits before the socket exists.
// logPath is the --log file this run created; it is read only on that early exit.
bool waitAndRelaxPerms(uid_t clientUid, pid_t ovpnPid, const std::string &logPath, std::string &outDetail);

} // namespace OvpnMgmt
