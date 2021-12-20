#include "installedantiviruses_win.h"
#include <comdef.h>
#include "Utils/logger.h"

#pragma comment(lib, "wbemuuid.lib")

QList<InstalledAntiviruses_win::AntivirusInfo> InstalledAntiviruses_win::list;


void InstalledAntiviruses_win::outToLog()
{
    list.clear();
    getSecurityCenter(L"ROOT\\SecurityCenter");
    getSecurityCenter(L"ROOT\\SecurityCenter2");

    QList<AntivirusInfo> listAntiViruses, listSpywares, listFirewalls;
    for (const AntivirusInfo &ai : qAsConst(list))
    {
        if (ai.productType == PT_SPYWARE)
        {
            listSpywares << ai;
        }
        else if (ai.productType == PT_ANTIVIRUS)
        {
            listAntiViruses << ai;
        }
        else if (ai.productType == PT_FIREWALL)
        {
            listFirewalls << ai;
        }
        else
        {
            Q_ASSERT(false);
        }
    }

    if (listSpywares.count() > 0)
    {
        qCDebug(LOG_BASIC) << "Detected AntiSpyware products:" << makeStrFromList(listSpywares);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Detected AntiSpyware products: empty";
    }
    if (listAntiViruses.count() > 0)
    {
        qCDebug(LOG_BASIC) << "Detected AntiVirus products:" << makeStrFromList(listAntiViruses);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Detected AntiVirus products: empty";
    }
    if (listFirewalls.count() > 0)
    {
        qCDebug(LOG_BASIC) << "Detected Firewall products:" <<  makeStrFromList(listFirewalls);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Detected Firewall products: empty";
    }
}

void InstalledAntiviruses_win::getSecurityCenter(const wchar_t *path)
{
    HRESULT hres;

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);

    if (FAILED(hres))
    {
        return;
    }

    IWbemServices *pSvc = NULL;

    hres = pLoc->ConnectServer(_bstr_t(path), NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hres))
    {
        pLoc->Release();
        return;
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
            NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        return;
    }

    list.append(enumField(pSvc, "SELECT * FROM AntiVirusProduct", PT_ANTIVIRUS));
    list.append(enumField(pSvc, "SELECT * FROM AntiSpywareProduct", PT_SPYWARE));
    list.append(enumField(pSvc, "SELECT * FROM FirewallProduct", PT_FIREWALL));

    pSvc->Release();
    pLoc->Release();
}

QList<InstalledAntiviruses_win::AntivirusInfo> InstalledAntiviruses_win::enumField(IWbemServices *pSvc,
                                              const char *request, PRODUCT_TYPE productType)
{
    QList<AntivirusInfo> listAv;

    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(request),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

    if (FAILED(hres))
    {
        return listAv;
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

       VARIANT vtProp;

       AntivirusInfo ai;
       // Get the value of the Name property
       hr = pclsObj->Get(L"displayName", 0, &vtProp, 0, 0);
       if (FAILED(hr))
       {
           continue;
       }
       ai.name = QString::fromWCharArray(vtProp.bstrVal);
       VariantClear(&vtProp);

       // Get the value of the State property
       hr = pclsObj->Get(L"productState", 0, &vtProp, 0, 0);
       if (SUCCEEDED(hr))
       {
           ai.state = vtProp.uintVal;
           ai.bStateAvailable = true;
           VariantClear(&vtProp);
       }

       ai.productType = productType;

       listAv << ai;

       pclsObj->Release();
    }
    return listAv;
}

QString InstalledAntiviruses_win::makeStrFromList(const QList<InstalledAntiviruses_win::AntivirusInfo> &other_list)
{
    QString ret = "(";
    for (int i = 0; i < other_list.count(); ++i)
    {
        if (other_list.at(i).bStateAvailable)
        {
            ret += "name = " + other_list.at(i).name + ", state = " + QString::number(other_list.at(i).state) + " [" + recognizeState(other_list.at(i).state) + "]";
        }
        else
        {
            ret += "name = " + other_list.at(i).name;
        }
        if (i != other_list.count() - 1)
        {
            ret += "; ";
        }
    }
    ret += ")";
    return ret;
}

QString InstalledAntiviruses_win::recognizeState(quint32 state)
{
    QString hexValue = QString("%1").arg(state, 6, 16, QLatin1Char( '0' ));
    QString ret;
    if (hexValue.mid(2, 2) == "10" || hexValue.mid(2, 2) == "11")
    {
        ret += "enabled";
    }
    else if (hexValue.mid(2, 2) == "00" || hexValue.mid(2, 2) == "01")
    {
        ret += "disabled";
    }

    if (hexValue.mid(4, 2) == "00")
    {
        if (!ret.isEmpty())
        {
            ret += " ";
        }
        ret += "up-to-date";
    }
    else if (hexValue.mid(4, 2) == "10")
    {
        if (!ret.isEmpty())
        {
            ret += " ";
        }
        ret += "outdated";
    }

    return ret;
}

