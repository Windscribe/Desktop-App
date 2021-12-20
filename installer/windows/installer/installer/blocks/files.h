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
	Files(const std::wstring &installPath, double weight);
	virtual ~Files();

	virtual int executeStep();

 private:
   std::wstring installPath_;
   Archive *archive_;
   int state_;
   unsigned int curFileInd_;
   std::list<std::wstring> fileList_;
   std::list<std::wstring> pathList_;

   bool isWin64();
   void fillPathList();
   std::wstring getFileName(const std::wstring &s);
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
