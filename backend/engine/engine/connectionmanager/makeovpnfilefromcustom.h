#ifndef MAKEOVPNFILEFROMCUSTOM_H
#define MAKEOVPNFILEFROMCUSTOM_H
#include <QTemporaryFile>
#include "engine/types/protocoltype.h"

class MakeOVPNFileFromCustom
{
public:
    MakeOVPNFileFromCustom();
    virtual ~MakeOVPNFileFromCustom();

    bool generate(const QString &ovpnData, const QString &ip);
    QString path() const { return path_; }
    QString protocol() const;
    uint port() const {return port_;}

private:
    QString path_;
    QFile file_;
    uint port_;
};

#endif // MAKEOVPNFILEFROMCUSTOM_H
