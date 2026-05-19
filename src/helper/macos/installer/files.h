#pragma once

#include <string>

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
    std::string archiveTempPath_;
    std::string lastError_;
};
