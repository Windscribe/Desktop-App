#pragma once

#include <memory>
#include <string>

// Service-account access to the local directory node. Attribute names are OpenDirectory's full form,
// e.g. "dsAttrTypeStandard:UniqueID". Node methods other than deleteRecord log their own failures; the lookups do not.
namespace Directory {

enum class RecordType { User, Group };
enum class Read { Found, Absent, Failed };

class Node
{
public:
    // Opens the local node once for the lifetime of the object; false afterwards means it could not be opened.
    Node();
    ~Node();
    explicit operator bool() const;

    // Absent covers both a missing record and a record without the attribute.
    Read readAttribute(RecordType type, const std::string &name, const std::string &attribute, std::string &value) const;
    // Number of local records of `type` holding exactly `value` for `attribute`, or -1 on failure.
    int countHolders(RecordType type, const std::string &attribute, const std::string &value) const;
    // Creates the record if it does not exist.
    bool setAttribute(RecordType type, const std::string &name, const std::string &attribute, const std::string &value) const;
    // Succeeds when the record is already absent.
    bool deleteRecord(RecordType type, const std::string &name) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// System name resolution, i.e. what a privilege drop or a pf rule will see. False when the name does not resolve.
bool lookupUser(const std::string &name, unsigned int &uid, unsigned int &primaryGid);
bool lookupGroup(const std::string &name, unsigned int &gid);

} // namespace Directory
