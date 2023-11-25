#include "files_manager.h"

void FilesManager::setFiles(Files *files)
{
    if (files_) {
        delete files_;
    }
    files_ = files;
}

Files *FilesManager::files()
{
    return files_;
}
