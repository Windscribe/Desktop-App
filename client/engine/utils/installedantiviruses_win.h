#ifndef INSTALLEDANTIVIRUSES_WIN_H
#define INSTALLEDANTIVIRUSES_WIN_H

#include <QList>
#include <QString>
#include <Wbemidl.h>

class InstalledAntiviruses_win
{
public:
    static void outToLog();

private:

    enum PRODUCT_TYPE { PT_SPYWARE, PT_ANTIVIRUS, PT_FIREWALL };

    struct AntivirusInfo
    {
        QString name;
        quint32 state;
        bool bStateAvailable;
        PRODUCT_TYPE productType;

        AntivirusInfo() : state(0), bStateAvailable(false), productType(PT_SPYWARE) {}
    };

    static void getSecurityCenter(const wchar_t *path);
    static QList<AntivirusInfo> enumField(IWbemServices *pSvc, const char *request, PRODUCT_TYPE productType);
    static QString makeStrFromList(const QList<AntivirusInfo> &other_list);

    static QList<AntivirusInfo> list;

    static QString recognizeState(quint32 state);
};
#endif // INSTALLEDANTIVIRUSES_WIN_H
