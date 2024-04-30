#pragma once

#include <list>
#include <string>

#include "../../../../client/common/archive/archive.h"

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
    Archive *archive_;
    int state_;
    unsigned int curFileInd_;
    std::list<std::wstring> fileList_;
    std::list<std::wstring> pathList_;
    std::string lastError_;

    void eraseSubStr(std::wstring &mainStr, const std::wstring &toErase);
    void fillPathList();
    std::wstring getFileName(const std::wstring &s);
};
