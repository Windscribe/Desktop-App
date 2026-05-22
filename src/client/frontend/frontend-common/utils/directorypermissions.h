#pragma once

#include <QString>

namespace DirectoryPermissions
{
    // Returns true if the directory at the given path can be written to by a non-admin
    // user other than the current user.  Inspects extended ACLs, mode bits, and
    // ownership on POSIX; inspects the DACL and owner on Windows.  Does not write
    // anything and does not require elevation.  Returns false on stat/ACL lookup failure
    // so that an inaccessible path doesn't produce a false warning.
    bool isWritableByOtherUsers(const QString &path);
}
