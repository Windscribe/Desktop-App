#ifndef FILES_H
#define FILES_H
#include <string>
#include <list>
#include "../iinstall_block.h"
#include "../../Archive/archive.h"
#include "../../../Utils/path.h"
#include "../../../Utils/registry.h"
#include "../../../Utils/process1.h"

#include <VersionHelpers.h>

class Files : public IInstallBlock
{
public:
	Files(double weight);
	virtual ~Files();

	virtual int executeStep();

 private:
   Archive *archive_ = nullptr;
   int state_ = 0;
   unsigned int curFileInd_ = 0;
   std::list<std::wstring> fileList_;
   std::list<std::wstring> pathList_;
   std::wstring installPath_;

   bool isWin64();
   void fillPathList();
   std::wstring getFileName(const std::wstring &s);
   int moveFiles();
};


class FilterFiles
{
public:
	FilterFiles(bool isX64, bool win10_or_greater);
	bool operator()(const std::wstring &value) const;

private:
	bool isX64_;
	bool win10_or_greater_;
};

#endif // FILES_H
