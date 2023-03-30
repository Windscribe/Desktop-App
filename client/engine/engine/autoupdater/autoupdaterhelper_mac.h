#ifndef AUTOUPDATERHELPER_H
#define AUTOUPDATERHELPER_H

#include <QString>
#include "types/enums.h"

class AutoUpdaterHelper_mac
{
public:
    explicit AutoUpdaterHelper_mac();

    const QString copyInternalInstallerToTempFromDmg(const QString &dmgFilename);
    bool verifyAndRun(const QString &tempInstallerFilename, const QString &additionalArgs);
    UPDATE_VERSION_ERROR error();

private:
    const QString mountDmg(const QString &dmgFilename);
    bool unmountVolume(const QString &volumePath);

    UPDATE_VERSION_ERROR error_;
};

#endif // AUTOUPDATERHELPER_H
