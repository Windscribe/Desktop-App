#ifndef MAKEOVPNFILEFROMCUSTOM_H
#define MAKEOVPNFILEFROMCUSTOM_H
#include <QTemporaryFile>
#include "Engine/Types/protocoltype.h"

class MakeOVPNFileFromCustom
{
public:
    MakeOVPNFileFromCustom();
    virtual ~MakeOVPNFileFromCustom();

    bool generate(const QString &pathCustomOvpn, const QString &ip);
    QString path() { return path_; }
    uint port() {return port_;}

private:
    QString path_;
    QFile file_;
    uint port_;
};

#endif // MAKEOVPNFILEFROMCUSTOM_H
