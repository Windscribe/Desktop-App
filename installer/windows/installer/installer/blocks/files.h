#ifndef FILES_H
#define FILES_H

#include <list>
#include <string>

#include "../iinstall_block.h"
#include "../../Archive/archive.h"

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
   std::wstring getFileName(const std::wstring &s);
   int moveFiles();
};


class FilterFiles
{
public:
	FilterFiles(bool isX64, bool win10_or_greater);
	bool operator()(const std::wstring &value) const;

private:
	const bool isX64_;
	const bool win10_or_greater_;
};

#endif // FILES_H
