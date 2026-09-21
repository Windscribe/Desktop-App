#include "appuser.h"

#include <charconv>
#include <climits>
#include <spdlog/spdlog.h>
#include <string>

#include "directory_mac.h"

namespace {

using Directory::RecordType;

const std::string kUniqueID = "dsAttrTypeStandard:UniqueID";
const std::string kPrimaryGroupID = "dsAttrTypeStandard:PrimaryGroupID";

bool assignId(const Directory::Node &node, RecordType type, const std::string &attribute, unsigned int preferred,
              std::string &assigned)
{
    std::string value;
    const Directory::Read read = node.readAttribute(type, WS_PRODUCT_NAME_LOWER, attribute, value);
    if (read == Directory::Read::Failed) {
        return false;
    }
    if (read == Directory::Read::Found) {
        long long existing = 0;
        const char *end = value.data() + value.size();
        auto parsed = std::from_chars(value.data(), end, existing);
        if (parsed.ec != std::errc() || parsed.ptr != end || existing > UINT_MAX) {
            spdlog::error("Account provisioning: invalid {} for {}: {}", attribute, WS_PRODUCT_NAME_LOWER, value);
            return false;
        }
        if (existing <= 0) {
            spdlog::error("Account provisioning: refusing nonpositive {} for {}", attribute, WS_PRODUCT_NAME_LOWER);
            return false;
        }
        const std::string id = std::to_string(existing);
        const int count = node.countHolders(type, attribute, id);
        if (count < 0) {
            return false;
        }
        if (count == 1) {
            assigned = id;
            return true;
        }
        spdlog::warn("Account provisioning: {} {} has {} holders, reassigning", attribute, id, count);
    }
    // Each probe is a directory query, so the walk is bounded instead of running to the end of the range.
    for (unsigned int candidate = preferred; candidate < preferred + 32; ++candidate) {
        int count = node.countHolders(type, attribute, std::to_string(candidate));
        if (count < 0) {
            return false;
        }
        if (count != 0) {
            continue;
        }
        assigned = std::to_string(candidate);
        if (!node.setAttribute(type, WS_PRODUCT_NAME_LOWER, attribute, assigned)) {
            return false;
        }
        count = node.countHolders(type, attribute, assigned);
        if (count < 0) {
            return false;
        }
        if (count != 1) {
            spdlog::error("Account provisioning: {} {} has {} holders after assignment", attribute, assigned, count);
            return false;
        }
        return true;
    }
    spdlog::error("Account provisioning: no available {}", attribute);
    return false;
}

} // namespace

void AppUser::createUserAndGroup()
{
    Directory::Node node;
    if (!node) {
        return;
    }
    std::string gid, uid;
    auto set = [&node](RecordType type, const std::string &attribute, const std::string &value) {
        return node.setAttribute(type, WS_PRODUCT_NAME_LOWER, attribute, value);
    };
    // Always attempt to recreate group/user, even if they exist.
    if (!assignId(node, RecordType::Group, kPrimaryGroupID, std::stoul(WS_MAC_GID), gid) ||
        !set(RecordType::Group, "dsAttrTypeStandard:Password", "*") ||
        !set(RecordType::Group, "dsAttrTypeStandard:GroupMembership", WS_PRODUCT_NAME_LOWER) ||
        !set(RecordType::Group, "dsAttrTypeStandard:RealName", WS_PRODUCT_NAME " Apps Group") ||
        !set(RecordType::User, "dsAttrTypeNative:IsHidden", "1") ||
        !set(RecordType::User, kPrimaryGroupID, gid) ||
        !assignId(node, RecordType::User, kUniqueID, std::stoul(WS_MAC_UID), uid) ||
        !set(RecordType::User, "dsAttrTypeStandard:Password", "*") ||
        !set(RecordType::User, "dsAttrTypeStandard:RealName", WS_PRODUCT_NAME " Apps User") ||
        !set(RecordType::User, "dsAttrTypeStandard:UserShell", "/bin/false")) {
        return;
    }
    unsigned int resolvedUid = 0, resolvedGid = 0;
    if (!Directory::lookupUser(WS_PRODUCT_NAME_LOWER, resolvedUid, resolvedGid)) {
        spdlog::error("Account provisioning: user {} does not resolve", WS_PRODUCT_NAME_LOWER);
        return;
    }
    if (std::to_string(resolvedUid) != uid || std::to_string(resolvedGid) != gid) {
        spdlog::error("Account provisioning: user {} resolves to {}:{}, expected {}:{}", WS_PRODUCT_NAME_LOWER, resolvedUid, resolvedGid,
                      uid, gid);
        return;
    }
    if (!Directory::lookupGroup(WS_PRODUCT_NAME_LOWER, resolvedGid)) {
        spdlog::error("Account provisioning: group {} does not resolve", WS_PRODUCT_NAME_LOWER);
    } else if (std::to_string(resolvedGid) != gid) {
        spdlog::error("Account provisioning: group lookup returned {}, expected GID {}", resolvedGid, gid);
    }
}
