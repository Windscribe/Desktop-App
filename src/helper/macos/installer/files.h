#pragma once

#include <string>

class Files
{
public:
    Files(const std::wstring &archivePath, const std::wstring &installPath);
    ~Files();

    int executeStep();
    std::string getLastError() { return lastError_; }

private:
    std::wstring archivePath_;
    std::wstring installPath_;
    std::string lastError_;
};
