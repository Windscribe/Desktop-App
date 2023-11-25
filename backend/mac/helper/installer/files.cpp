#include "files.h"
#include "../logger.h"

Files::Files(const std::wstring &archivePath, const std::wstring &installPath, uid_t userId, gid_t groupId) : userId_(userId),
    groupId_(groupId), archive_(NULL),
    archivePath_(archivePath), installPath_(installPath), state_(0)
{
}

Files::~Files()
{
    if (archive_)
    {
        delete archive_;
    }
}

int Files::executeStep()
{
    if (state_ == 0)
    {
        if (archive_)
        {
            delete archive_;
        }

        archive_ = new Archive(archivePath_, userId_, groupId_);
        if (!archive_->isCorrect())
        {
            LOG("Can't open file archive");
            lastError_ = "Can't open file archive.";
            return -1;
        }

        SRes res = archive_->fileList(fileList_);
        if (res != SZ_OK)
        {
            LOG("Can't get files list from archive");
            lastError_ = "Can't get files list from archive.";
            return -1;
        }

        fillPathList();

        archive_->calcTotal(fileList_, pathList_);
        curFileInd_ = 0;
        state_++;
        return 0;
    }
    else
    {
        SRes res = archive_->extractionFile(curFileInd_);
        if (res != SZ_OK)
        {
            archive_->finish();
            LOG("Can't extract files");
            lastError_ = "Can't extract file.";
            return -1;
        }
        else
        {
            int progress = ((double)curFileInd_ / (double)archive_->getNumFiles()) * 100.0;

            if (curFileInd_ >= (archive_->getNumFiles() - 1))
            {
                archive_->finish();
                return 100;
            }
            else
            {
                curFileInd_++;
            }
            return progress;
        }
    }

    return 100;
}

void Files::eraseSubStr(std::wstring &mainStr, const std::wstring &toErase)
{
    size_t pos = mainStr.find(toErase);
    if (pos != std::string::npos)
    {
        mainStr.erase(pos, toErase.length());
    }
}

void Files::fillPathList()
{
    pathList_.clear();
    for (auto it = fileList_.cbegin(); it != fileList_.cend(); it++)
    {
        std::wstring srcPath = *it;
        eraseSubStr(srcPath, L"Windscribe.app/");

        std::wstring destination = installPath_ + L"/" + srcPath;
        std::wstring directory;
        const size_t last_slash_idx = destination.rfind(L'/');
        if (std::wstring::npos != last_slash_idx)
        {
            directory = destination.substr(0, last_slash_idx);
        }

        pathList_.push_back(directory);
    }
}


std::wstring Files::getFileName(const std::wstring &s)
{
    std::wstring str;

    size_t i = s.rfind('\\', s.length());

    if (i == std::wstring::npos)
    {
        i = s.rfind('/', s.length());
    }


    if (i != std::wstring::npos)
    {
        str = s.substr(i + 1, s.length() - i);
    }

    return str;
}
