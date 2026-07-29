#pragma once

#include <string>
#include <vector>

class Files
{
public:
    // archiveTempPath is a root-owned temp file containing the staged installer
    // archive. The destination is hardcoded to WS_MAC_APP_DIR. The temp file is
    // unlinked when this Files instance is destroyed.
    explicit Files(const std::string &archiveTempPath);
    ~Files();

    int executeStep();
    std::string getLastError() { return lastError_; }

private:
    // Extract selected payload members into a root-only directory (root:wheel 0755), replacing
    // whatever was there. Used for the copies of dns.sh and the bundled executables that the
    // helper runs as root, which must not come from the app bundle.
    bool extractToRootOnlyDir(const std::string &dir, const std::vector<std::string> &members);

    std::string archiveTempPath_;
    std::string lastError_;
};
