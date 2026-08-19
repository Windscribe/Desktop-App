#pragma once

#include <QString>
#include "types/enums.h"

class AutoUpdaterHelper_mac
{
public:
    explicit AutoUpdaterHelper_mac();

    bool verifyAndRun(const QString &tempInstallerFilename, const QString &additionalArgs);
    UPDATE_VERSION_ERROR error();

private:
    UPDATE_VERSION_ERROR error_;
};
