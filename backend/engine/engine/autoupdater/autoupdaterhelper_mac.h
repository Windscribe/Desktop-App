#ifndef AUTOUPDATERHELPER_H
#define AUTOUPDATERHELPER_H

#include <QString>
#include "ipc/generated_proto/types.pb.h"

class AutoUpdaterHelper_mac
{
public:
    explicit AutoUpdaterHelper_mac();

    const QString copyInternalInstallerToTempFromDmg(const QString &dmgFilename);
    bool verifyAndRun(const QString &tempInstallerFilename);
    ProtoTypes::UpdateVersionError error();

private:
    const QString mountDmg(const QString &dmgFilename);
    bool unmountVolume(const QString &volumePath);

    ProtoTypes::UpdateVersionError error_;
};

#endif // AUTOUPDATERHELPER_H
