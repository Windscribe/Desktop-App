#pragma once

#include <list>
#include <string>

#include "../iinstall_block.h"
#include "../../../../../client/common/archive/archive.h"

class Files : public IInstallBlock
{
public:
    Files(double weight);
    virtual ~Files();

    virtual int executeStep();

 private:
   std::unique_ptr<Archive> archive_;
   int state_ = 0;
   unsigned int curFileInd_ = 0;
   std::list<std::wstring> fileList_;
   std::list<std::wstring> pathList_;
   std::wstring installPath_;

   void fillPathList();
   int moveFiles();
   bool copyLibs();
   std::wstring getExePath();
};
