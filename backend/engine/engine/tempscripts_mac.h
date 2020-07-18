#ifndef TEMPSCRIPTS_MAC_H
#define TEMPSCRIPTS_MAC_H

#include <QString>

class TempScripts_mac
{

public:
    static TempScripts_mac &instance()
    {
        static TempScripts_mac i;
        return i;
    }

    QString dnsScriptPath();
    QString removeHostsScriptPath();


private:
    TempScripts_mac();
    ~TempScripts_mac();

    QString dnsScriptPath_;
    QString removeHostsScriptPath_;
};

#endif // TEMPSCRIPTS_MAC_H
