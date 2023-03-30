#ifndef MAKEOVPNFILEFROMCUSTOM_H
#define MAKEOVPNFILEFROMCUSTOM_H

#include <QString>

class MakeOVPNFileFromCustom
{
public:
    MakeOVPNFileFromCustom();
    virtual ~MakeOVPNFileFromCustom();

    bool generate(const QString &customConfigPath, const QString &ovpnData, const QString &ip, const QString &remoteCommand);
    QString config() const { return config_; }

private:
    QString config_;
};

#endif // MAKEOVPNFILEFROMCUSTOM_H
