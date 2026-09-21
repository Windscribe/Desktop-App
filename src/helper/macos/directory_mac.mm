#include "directory_mac.h"

#import <Foundation/Foundation.h>
#import <OpenDirectory/OpenDirectory.h>

#include <grp.h>
#include <pwd.h>
#include <spdlog/spdlog.h>

namespace {

NSString *toNS(const std::string &s)
{
    return [NSString stringWithUTF8String:s.c_str()];
}

std::string describe(NSError *error)
{
    return error ? std::string(error.localizedDescription.UTF8String) : std::string("unknown error");
}

ODRecordType recordType(Directory::RecordType type)
{
    return type == Directory::RecordType::User ? kODRecordTypeUsers : kODRecordTypeGroups;
}

// Local records of `type` whose `attribute` equals `value` exactly, or nil on failure.
NSArray<ODRecord *> *findRecords(ODNode *node, Directory::RecordType type, NSString *attribute, NSString *value)
{
    if (!node || !attribute || !value) {
        return nil;
    }
    NSError *error = nil;
    ODQuery *query = [ODQuery queryWithNode:node
                             forRecordTypes:recordType(type)
                                  attribute:attribute
                                  matchType:kODMatchEqualTo
                                queryValues:value
                           returnAttributes:kODAttributeTypeStandardOnly
                             maximumResults:0
                                      error:&error];
    NSArray<ODRecord *> *results = query ? [query resultsAllowingPartial:NO error:&error] : nil;
    if (!results) {
        spdlog::error("Account provisioning: directory query {} = {} failed: {}", attribute.UTF8String, value.UTF8String, describe(error));
    }
    return results;
}

} // namespace

namespace Directory {

struct Node::Impl {
    ODNode *node = nil;
};

// The search node is deliberately not used: a bound Mac's directory must never have to answer for the
// helper to provision its own local account.
Node::Node() : impl_(std::make_unique<Impl>())
{
    NSError *error = nil;
    impl_->node = [ODNode nodeWithSession:[ODSession defaultSession] type:kODNodeTypeLocalNodes error:&error];
    if (!impl_->node) {
        spdlog::error("Account provisioning: local directory node unavailable: {}", describe(error));
    }
}

Node::~Node() = default;

Node::operator bool() const
{
    return impl_->node != nil;
}

Read Node::readAttribute(RecordType type, const std::string &name, const std::string &attribute, std::string &value) const
{
    NSArray<ODRecord *> *records = findRecords(impl_->node, type, kODAttributeTypeRecordName, toNS(name));
    if (!records) {
        return Read::Failed;
    }
    if (records.count == 0) {
        return Read::Absent;
    }
    NSError *error = nil;
    NSString *key = toNS(attribute);
    NSDictionary<NSString *, NSArray *> *details = [records.firstObject recordDetailsForAttributes:@[key] error:&error];
    if (!details) {
        spdlog::error("Account provisioning: reading {} of {} failed: {}", attribute, name, describe(error));
        return Read::Failed;
    }
    NSArray *values = details[key];
    if (values.count == 0) {
        return Read::Absent;
    }
    // Several values, or a non-string one, are passed through in a form the caller's numeric parse rejects.
    value.clear();
    for (id item in values) {
        value += (value.empty() ? "" : " ");
        value += [item isKindOfClass:[NSString class]] ? [item UTF8String] : "<binary>";
    }
    return Read::Found;
}

int Node::countHolders(RecordType type, const std::string &attribute, const std::string &value) const
{
    NSArray<ODRecord *> *records = findRecords(impl_->node, type, toNS(attribute), toNS(value));
    return records ? static_cast<int>(records.count) : -1;
}

bool Node::setAttribute(RecordType type, const std::string &name, const std::string &attribute, const std::string &value) const
{
    NSArray<ODRecord *> *records = findRecords(impl_->node, type, kODAttributeTypeRecordName, toNS(name));
    if (!records) {
        return false;
    }
    NSError *error = nil;
    ODRecord *record = records.firstObject;
    if (!record) {
        record = [impl_->node createRecordWithRecordType:recordType(type) name:toNS(name) attributes:nil error:&error];
        if (!record) {
            spdlog::error("Account provisioning: creating {} failed: {}", name, describe(error));
            return false;
        }
    }
    if (![record setValue:toNS(value) forAttribute:toNS(attribute) error:&error]) {
        spdlog::error("Account provisioning: setting {} on {} failed: {}", attribute, name, describe(error));
        return false;
    }
    return true;
}

bool Node::deleteRecord(RecordType type, const std::string &name) const
{
    NSArray<ODRecord *> *records = findRecords(impl_->node, type, kODAttributeTypeRecordName, toNS(name));
    if (!records) {
        return false;
    }
    if (records.count == 0) {
        return true;
    }
    return [records.firstObject deleteRecordAndReturnError:nil];
}

bool lookupUser(const std::string &name, unsigned int &uid, unsigned int &primaryGid)
{
    const struct passwd *pw = getpwnam(name.c_str());
    if (!pw) {
        return false;
    }
    uid = pw->pw_uid;
    primaryGid = pw->pw_gid;
    return true;
}

bool lookupGroup(const std::string &name, unsigned int &gid)
{
    const struct group *gr = getgrnam(name.c_str());
    if (!gr) {
        return false;
    }
    gid = gr->gr_gid;
    return true;
}

} // namespace Directory
