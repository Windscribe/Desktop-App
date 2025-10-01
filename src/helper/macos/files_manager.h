#pragma once

#include <string>
#include "installer/files.h"

class FilesManager
{
public:
    static FilesManager& instance()
    {
        static FilesManager fm;
        return fm;
    }
    void setFiles(Files *files);
    Files *files();

private:
    FilesManager() : files_(nullptr) {};
    ~FilesManager() {};

    Files *files_;
};
