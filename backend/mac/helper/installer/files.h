#pragma once

#include <string>
#include <list>
#include "archive/archive.h"

class Files
{
public:
    Files(const std::wstring &archivePath, const std::wstring &installPath, uid_t userId, gid_t groupId);
    ~Files();

    int executeStep();
    std::string getLastError() { return lastError_; }

 private:
   uid_t userId_;
    gid_t groupId_;
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
