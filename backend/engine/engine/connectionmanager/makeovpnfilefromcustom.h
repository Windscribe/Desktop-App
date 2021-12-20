#ifndef MAKEOVPNFILEFROMCUSTOM_H
#define MAKEOVPNFILEFROMCUSTOM_H
#include <QTemporaryFile>
#include "engine/types/protocoltype.h"

class MakeOVPNFileFromCustom
{
public:
    MakeOVPNFileFromCustom();
    virtual ~MakeOVPNFileFromCustom();

    bool generate(const QString &customConfigPath, const QString &ovpnData, const QString &ip, const QString &remoteCommand);
    QString path() const { return path_; }

private:
    QString path_;
    QFile file_;
};

#endif // MAKEOVPNFILEFROMCUSTOM_H
