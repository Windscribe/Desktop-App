#include "../appuser.h"

#include <cstdio>
#include <map>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>

#include "../directory_mac.h"

namespace {
using Directory::RecordType;
using Attributes = std::map<std::string, std::string>;
std::map<std::string, Attributes> records;
std::ostringstream logs;
int calls, failAt, racers, verifications, failures, idWrites;
int wrongLookup;
bool preferredTaken, rangeTaken, nodeUnavailable;
const std::string kUniqueID = "dsAttrTypeStandard:UniqueID";
const std::string kPrimaryGroupID = "dsAttrTypeStandard:PrimaryGroupID";
const std::string user = "/Users/" WS_PRODUCT_NAME_LOWER;
const std::string group = "/Groups/" WS_PRODUCT_NAME_LOWER;

void check(bool ok, int line)
{
    if (!ok) {
        std::printf("FAIL line %d\n", line);
        ++failures;
    }
}
#define CHECK(expr) check((expr), __LINE__)

std::string path(RecordType type, const std::string &name)
{
    return (type == RecordType::User ? "/Users/" : "/Groups/") + name;
}

bool injected()
{
    if (++calls != failAt) {
        return false;
    }
    spdlog::error("injected failure");
    return true;
}

void reset()
{
    records = {{"/Users/nobody", {{kUniqueID, "-2"}}}, {"/Groups/nobody", {{kPrimaryGroupID, "-2"}}}};
    logs.str("");
    calls = failAt = racers = verifications = idWrites = 0;
    wrongLookup = 0;
    preferredTaken = true;
    rangeTaken = nodeUnavailable = false;
}

void verifyHealthy(const std::string &uid, const std::string &gid)
{
    CHECK(records[user][kUniqueID] == uid);
    CHECK(records[group][kPrimaryGroupID] == gid);
    CHECK(records[user][kPrimaryGroupID] == gid);
    CHECK(records[user]["dsAttrTypeStandard:UserShell"] == "/bin/false");
    CHECK(records[user]["dsAttrTypeStandard:Password"] == "*");
    CHECK(records[user]["dsAttrTypeNative:IsHidden"] == "1");
    CHECK(records[group]["dsAttrTypeStandard:GroupMembership"] == WS_PRODUCT_NAME_LOWER);
    CHECK(verifications == 2);
}
} // namespace

struct Directory::Node::Impl {};

Directory::Node::Node() : impl_(nullptr)
{
    if (nodeUnavailable) {
        spdlog::error("injected failure");
    }
}

Directory::Node::~Node() = default;

Directory::Node::operator bool() const
{
    return !nodeUnavailable;
}

Directory::Read Directory::Node::readAttribute(RecordType type, const std::string &name, const std::string &attribute,
                                               std::string &value) const
{
    if (injected()) {
        return Read::Failed;
    }
    auto record = records.find(path(type, name));
    if (record == records.end() || !record->second.count(attribute)) {
        return Read::Absent;
    }
    value = record->second[attribute];
    return Read::Found;
}

int Directory::Node::countHolders(RecordType type, const std::string &attribute, const std::string &value) const
{
    if (injected()) {
        return -1;
    }
    int count = 0;
    for (const auto &[record, attrs] : records) {
        auto it = attrs.find(attribute);
        if (record.rfind(path(type, ""), 0) == 0 && it != attrs.end() && it->second == value) {
            ++count;
        }
    }
    if (preferredTaken && value == (type == RecordType::User ? WS_MAC_UID : WS_MAC_GID)) {
        ++count;
    }
    return count + (rangeTaken ? 1 : 0);
}

bool Directory::Node::setAttribute(RecordType type, const std::string &name, const std::string &attribute,
                                   const std::string &value) const
{
    if (injected()) {
        return false;
    }
    const std::string record = path(type, name);
    if ((record == user && attribute == kUniqueID) || (record == group && attribute == kPrimaryGroupID)) {
        ++idWrites;
        if (racers > 0) {
            --racers;
            records[path(type, "racer")][attribute] = value;
        }
    }
    records[record][attribute] = value;
    return true;
}

bool Directory::lookupUser(const std::string &name, unsigned int &uid, unsigned int &primaryGid)
{
    if (injected()) {
        return false;
    }
    ++verifications;
    uid = wrongLookup == 1 ? 99999 : std::stoul(records[path(RecordType::User, name)][kUniqueID]);
    primaryGid = wrongLookup == 2 ? 99999 : std::stoul(records[path(RecordType::User, name)][kPrimaryGroupID]);
    return true;
}

bool Directory::lookupGroup(const std::string &name, unsigned int &gid)
{
    if (injected()) {
        return false;
    }
    ++verifications;
    gid = wrongLookup == 3 ? 99999 : std::stoul(records[path(RecordType::Group, name)][kPrimaryGroupID]);
    return true;
}

int main()
{
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::ostream_sink_mt>(logs)));
    const std::string uid = std::to_string(std::stoul(WS_MAC_UID) + 1);
    const std::string gid = std::to_string(std::stoul(WS_MAC_GID) + 1);
    reset();
    AppUser::createUserAndGroup();
    verifyHealthy(uid, gid);
    CHECK(logs.str().empty());
    const int totalCalls = calls;
    idWrites = verifications = 0;
    AppUser::createUserAndGroup();
    verifyHealthy(uid, gid);
    CHECK(idWrites == 0 && logs.str().empty());
    for (const auto &record : {user, group}) {
        records[record].erase(record == user ? kUniqueID : kPrimaryGroupID);
        idWrites = verifications = 0;
        AppUser::createUserAndGroup();
        verifyHealthy(uid, gid);
        CHECK(idWrites == 1 && logs.str().empty());
    }
    for (int failure = 1; failure <= totalCalls; ++failure) {
        reset();
        failAt = failure;
        AppUser::createUserAndGroup();
        CHECK(calls == failAt);
        CHECK(logs.str().find("injected failure") != std::string::npos);
    }
    for (const auto &collision : {std::string(), user, group}) {
        reset();
        AppUser::createUserAndGroup();
        if (!collision.empty()) {
            const std::string attribute = collision == user ? kUniqueID : kPrimaryGroupID;
            records[collision == user ? "/Users/other" : "/Groups/other"][attribute] = records[collision][attribute];
        }
        const auto initialRecords = records;
        calls = 0;
        AppUser::createUserAndGroup();
        const int populatedCalls = calls;
        for (int failure = 1; failure <= populatedCalls; ++failure) {
            reset();
            records = initialRecords;
            failAt = failure;
            AppUser::createUserAndGroup();
            CHECK(calls == failAt);
            CHECK(logs.str().find("injected failure") != std::string::npos);
        }
    }
    for (const auto &record : {user, group}) {
        reset();
        AppUser::createUserAndGroup();
        const std::string attribute = record == user ? kUniqueID : kPrimaryGroupID;
        records[record][attribute] = "0" + records[record][attribute];
        logs.str("");
        idWrites = verifications = 0;
        AppUser::createUserAndGroup();
        verifyHealthy(uid, gid);
        CHECK(idWrites == 1 && logs.str().find("has 0 holders, reassigning") != std::string::npos);
    }
    reset();
    nodeUnavailable = true;
    AppUser::createUserAndGroup();
    CHECK(calls == 0 && idWrites == 0 && logs.str().find("injected failure") != std::string::npos);
    reset();
    AppUser::createUserAndGroup();
    records["/Users/other"][kUniqueID] = uid;
    idWrites = verifications = 0;
    AppUser::createUserAndGroup();
    verifyHealthy(std::to_string(std::stoul(uid) + 1), gid);
    CHECK(idWrites == 1 && logs.str().find("reassigning") != std::string::npos);
    records["/Groups/other"][kPrimaryGroupID] = gid;
    idWrites = verifications = 0;
    logs.str("");
    AppUser::createUserAndGroup();
    verifyHealthy(std::to_string(std::stoul(uid) + 1), std::to_string(std::stoul(gid) + 1));
    CHECK(idWrites == 1 && logs.str().find("reassigning") != std::string::npos);
    reset();
    rangeTaken = true;
    AppUser::createUserAndGroup();
    CHECK(calls == 1 + 32 && idWrites == 0 && logs.str().find("no available") != std::string::npos);
    reset();
    racers = 1;
    AppUser::createUserAndGroup();
    CHECK(idWrites == 1 && verifications == 0 && logs.str().find("after assignment") != std::string::npos);
    for (int lookup = 1; lookup <= 3; ++lookup) {
        reset();
        wrongLookup = lookup;
        AppUser::createUserAndGroup();
        CHECK(verifications == (lookup == 3 ? 2 : 1) && logs.str().find("expected") != std::string::npos);
    }
    reset();
    preferredTaken = false;
    AppUser::createUserAndGroup();
    verifyHealthy(WS_MAC_UID, WS_MAC_GID);
    idWrites = verifications = 0;
    AppUser::createUserAndGroup();
    verifyHealthy(WS_MAC_UID, WS_MAC_GID);
    CHECK(idWrites == 0 && logs.str().empty());
    for (const auto &largeId : {"2147483648", "3000000000", "4294967295"}) {
        reset();
        records[user][kUniqueID] = largeId;
        records[group][kPrimaryGroupID] = largeId;
        AppUser::createUserAndGroup();
        verifyHealthy(largeId, largeId);
        CHECK(idWrites == 0 && logs.str().empty());
    }
    for (const auto &record : {user, group}) {
        const std::string attribute = record == user ? kUniqueID : kPrimaryGroupID;
        for (const auto &invalidId : {"invalid", "4294967296", "9223372036854775808", "1639 1640"}) {
            reset();
            records[record][attribute] = invalidId;
            AppUser::createUserAndGroup();
            CHECK(verifications == 0 && logs.str().find("invalid") != std::string::npos);
        }
        for (const auto &nonpositiveId : {"0", "-1", "-2147483649"}) {
            reset();
            records[record][attribute] = nonpositiveId;
            AppUser::createUserAndGroup();
            CHECK(verifications == 0 && logs.str().find("nonpositive") != std::string::npos);
        }
    }
    return failures == 0 ? 0 : 1;
}
