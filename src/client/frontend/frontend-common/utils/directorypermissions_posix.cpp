#include "directorypermissions.h"

#include <sys/acl.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(Q_OS_LINUX)
#include <acl/libacl.h>
#elif defined(Q_OS_MACOS)
#include <membership.h>
#include <uuid/uuid.h>
#endif

namespace DirectoryPermissions
{

namespace {

// Extended ACLs (POSIX.1e on Linux, NFSv4-style on macOS) can grant write access that
// the mode bits don't reflect.  On Linux we treat any extended entry as potentially risky
// (POSIX.1e entries are grants by construction).  On macOS we must inspect the entries,
// because the OS installs a default "everyone deny delete" ACL on home subdirectories
// (Downloads, Documents, etc.) — those are protective restrictions, not write grants,
// and naively flagging any ACL produces false positives for normal user folders.
bool grantsWriteToOtherUsers(const QString &path)
{
    const QByteArray p = path.toUtf8();
#if defined(Q_OS_LINUX)
    return acl_extended_file(p.constData()) == 1;
#elif defined(Q_OS_MACOS)
    acl_t acl = acl_get_file(p.constData(), ACL_TYPE_EXTENDED);
    if (!acl) {
        return false;
    }

    uuid_t myUuid;
    const bool haveMyUuid = (mbr_uid_to_uuid(getuid(), myUuid) == 0);

    bool writable = false;
    acl_entry_t entry;
    for (int id = ACL_FIRST_ENTRY; acl_get_entry(acl, id, &entry) == 0; id = ACL_NEXT_ENTRY) {
        acl_tag_t tag;
        if (acl_get_tag_type(entry, &tag) != 0 || tag != ACL_EXTENDED_ALLOW) {
            continue;
        }

        acl_permset_t permset;
        if (acl_get_permset(entry, &permset) != 0) {
            continue;
        }
        const bool grantsWrite =
            acl_get_perm_np(permset, ACL_ADD_FILE) == 1 ||
            acl_get_perm_np(permset, ACL_ADD_SUBDIRECTORY) == 1 ||
            acl_get_perm_np(permset, ACL_DELETE_CHILD) == 1 ||
            acl_get_perm_np(permset, ACL_WRITE_DATA) == 1;
        if (!grantsWrite) {
            continue;
        }

        void *qualifier = acl_get_qualifier(entry);
        if (!qualifier) {
            writable = true;
            break;
        }
        const bool isMe = haveMyUuid &&
                          uuid_compare(*static_cast<uuid_t *>(qualifier), myUuid) == 0;
        acl_free(qualifier);
        if (!isMe) {
            writable = true;
            break;
        }
    }

    acl_free(acl);
    return writable;
#else
    Q_UNUSED(p);
    return false;
#endif
}

} // namespace

bool isWritableByOtherUsers(const QString &path)
{
    if (grantsWriteToOtherUsers(path)) {
        return true;
    }

    struct stat st;
    if (stat(path.toUtf8().constData(), &st) != 0) {
        return false;
    }

    const uid_t myUid = getuid();

    if (st.st_mode & S_IWOTH) {
        return true;
    }

    // Owner is someone other than the current user or root.  This includes system
    // daemon accounts (e.g. uid 1-999) — we flag those too since a user picking a
    // daemon-owned directory as their config dir is unusual and a legitimate concern.
    if (st.st_uid != myUid && st.st_uid != 0) {
        return true;
    }

    // Group-writable with a non-root group: the mode bit is the persistent security
    // setting regardless of current membership, and group membership is mutable.
    // Flag conservatively.
    if ((st.st_mode & S_IWGRP) && st.st_gid != 0) {
        return true;
    }

    return false;
}

} // namespace DirectoryPermissions
