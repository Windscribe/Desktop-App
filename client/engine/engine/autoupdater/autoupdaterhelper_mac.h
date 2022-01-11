#ifndef AUTOUPDATERHELPER_H
#define AUTOUPDATERHELPER_H

#include <QString>
#include "utils/protobuf_includes.h"

class AutoUpdaterHelper_mac
{
public:
    explicit AutoUpdaterHelper_mac();

    const QString copyInternalInstallerToTempFromDmg(const QString &dmgFilename);
    bool verifyAndRun(const QString &tempInstallerFilename, const QString &additionalArgs);
    ProtoTypes::UpdateVersionError error();

private:
    const QString mountDmg(const QString &dmgFilename);
    bool unmountVolume(const QString &volumePath);

    ProtoTypes::UpdateVersionError error_;
};

#endif // AUTOUPDATERHELPER_H
