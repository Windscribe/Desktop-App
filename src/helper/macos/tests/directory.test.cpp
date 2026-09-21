#include "../directory_mac.h"

#include <cstdio>
#include <string>

namespace {
using Directory::Read;
using Directory::RecordType;
int failures;
const std::string kUniqueID = "dsAttrTypeStandard:UniqueID";
const std::string kPrimaryGroupID = "dsAttrTypeStandard:PrimaryGroupID";
const std::string kRecordName = "dsAttrTypeStandard:RecordName";

void check(bool ok, int line)
{
    if (!ok) {
        std::printf("FAIL line %d\n", line);
        ++failures;
    }
}
#define CHECK(expr) check((expr), __LINE__)
} // namespace

int main()
{
    Directory::Node node;
    CHECK(static_cast<bool>(node));
    std::string value;
    CHECK(node.readAttribute(RecordType::User, "nobody", kUniqueID, value) == Read::Found && value == "-2");
    CHECK(node.readAttribute(RecordType::Group, "nobody", kPrimaryGroupID, value) == Read::Found && value == "-2");
    CHECK(node.readAttribute(RecordType::User, "root", kUniqueID, value) == Read::Found && value == "0");
    CHECK(node.readAttribute(RecordType::User, "nobody", "dsAttrTypeNative:IsHidden", value) == Read::Absent);
    CHECK(node.readAttribute(RecordType::User, "no_such_user_for_test", kUniqueID, value) == Read::Absent);
    CHECK(node.countHolders(RecordType::User, kUniqueID, "-2") == 1);
    CHECK(node.countHolders(RecordType::Group, kPrimaryGroupID, "-2") == 1);
    CHECK(node.countHolders(RecordType::User, kUniqueID, "1234567") == 0);
    CHECK(node.countHolders(RecordType::User, kRecordName, "_www") == 1);
    CHECK(node.countHolders(RecordType::User, kRecordName, "_ww") == 0);
    unsigned int uid = 0, gid = 0;
    CHECK(Directory::lookupUser("nobody", uid, gid) && uid == static_cast<unsigned int>(-2) && gid == static_cast<unsigned int>(-2));
    CHECK(Directory::lookupUser("root", uid, gid) && uid == 0 && gid == 0);
    CHECK(!Directory::lookupUser("no_such_user_for_test", uid, gid));
    CHECK(Directory::lookupGroup("nobody", gid) && gid == static_cast<unsigned int>(-2));
    CHECK(!Directory::lookupGroup("no_such_group_for_test", gid));
    return failures == 0 ? 0 : 1;
}
