#include "installedantiviruses_win.h"
#include <atlbase.h>
#include <comdef.h>
#include <netfw.h>
#include <QStringList>
#include "utils/ws_assert.h"
#include "utils/log/categories.h"

#pragma comment(lib, "wbemuuid.lib")

QList<InstalledAntiviruses_win::AntivirusInfo> InstalledAntiviruses_win::list;

void InstalledAntiviruses_win::outToLog()
{
    list.clear();
    getSecurityCenter();

    QList<AntivirusInfo> listAntiViruses, listSpywares, listFirewalls;
    for (const AntivirusInfo &ai : std::as_const(list)) {
        if (ai.productType == PT_SPYWARE) {
            listSpywares << ai;
        } else if (ai.productType == PT_ANTIVIRUS) {
            listAntiViruses << ai;
        } else if (ai.productType == PT_FIREWALL) {
            listFirewalls << ai;
        } else {
            WS_ASSERT(false);
        }
    }

    if (listSpywares.count() > 0) {
        qCInfo(LOG_BASIC).noquote() << "Detected AntiSpyware products:" << makeStrFromList(listSpywares);
    } else {
        qCInfo(LOG_BASIC) << "Detected AntiSpyware products: no third-party antispyware installed";
    }
    if (listAntiViruses.count() > 0) {
        qCInfo(LOG_BASIC).noquote() << "Detected AntiVirus products:" << makeStrFromList(listAntiViruses);
    } else {
        qCInfo(LOG_BASIC) << "Detected AntiVirus products: none found";
    }
    if (listFirewalls.count() > 0) {
        qCInfo(LOG_BASIC).noquote() << "Detected Firewall products:" <<  makeStrFromList(listFirewalls);
    } else {
        qCInfo(LOG_BASIC) << "Detected Firewall products: no third-party firewall installed";
    }

    logWindowsFirewallState();
}

void InstalledAntiviruses_win::logWindowsFirewallState()
{
    CComPtr<INetFwPolicy2> pNetFwPolicy2;
    HRESULT hr = pNetFwPolicy2.CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER);
    if (FAILED(hr)) {
        qCInfo(LOG_BASIC) << "Windows Firewall state: unavailable, hr =" << Qt::hex << hr;
        return;
    }

    // Reports which profiles are applied to the currently connected networks.  More than one bit
    // may be set when the machine is on multiple networks.
    long currentProfiles = 0;
    if (FAILED(pNetFwPolicy2->get_CurrentProfileTypes(&currentProfiles))) {
        currentProfiles = 0;
    }

    static const struct {
        NET_FW_PROFILE_TYPE2 type;
        const char *name;
    } kProfiles[] = {
        { NET_FW_PROFILE2_DOMAIN,  "domain"  },
        { NET_FW_PROFILE2_PRIVATE, "private" },
        { NET_FW_PROFILE2_PUBLIC,  "public"  },
    };

    QStringList states;
    for (const auto &profile : kProfiles) {
        QString state;
        VARIANT_BOOL enabled = VARIANT_FALSE;
        if (SUCCEEDED(pNetFwPolicy2->get_FirewallEnabled(profile.type, &enabled))) {
            state = (enabled != VARIANT_FALSE) ? "on" : "off";
        } else {
            state = "unknown";
        }
        if (currentProfiles & profile.type) {
            state += " [active]";
        }
        states << QString("%1 = %2").arg(QLatin1String(profile.name), state);
    }

    qCInfo(LOG_BASIC).noquote() << "Windows Firewall state: (" + states.join("; ") + ")";
}

void InstalledAntiviruses_win::getSecurityCenter()
{
    CComPtr<IWbemLocator> pLoc;
    HRESULT hres = pLoc.CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED(hres)) {
        return;
    }

    CComPtr<IWbemServices> pSvc;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\SecurityCenter2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        return;
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        return;
    }

    list.append(enumField(pSvc, "SELECT * FROM AntiVirusProduct", PT_ANTIVIRUS));
    list.append(enumField(pSvc, "SELECT * FROM AntiSpywareProduct", PT_SPYWARE));
    list.append(enumField(pSvc, "SELECT * FROM FirewallProduct", PT_FIREWALL));
}

QList<InstalledAntiviruses_win::AntivirusInfo> InstalledAntiviruses_win::enumField(IWbemServices *pSvc, const char *request, PRODUCT_TYPE productType)
{
    QList<AntivirusInfo> listAv;

    CComPtr<IEnumWbemClassObject> pEnumerator;
    HRESULT hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(request),
                                   WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                   NULL, &pEnumerator);
    if (FAILED(hres)) {
        return listAv;
    }

    while (true) {
        CComPtr<IWbemClassObject> pclsObj;
        ULONG uReturn = 0;

        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        // End of the enumeration returns WBEM_S_FALSE, which is a success code, so it falls
        // through to the uReturn check.
        if (FAILED(hr) || 0 == uReturn) {
            break;
        }

        VARIANT vtProp;

        AntivirusInfo ai;
        // Get the value of the Name property
        hr = pclsObj->Get(L"displayName", 0, &vtProp, 0, 0);
        if (FAILED(hr)) {
            continue;
        }
        if (vtProp.vt == VT_BSTR && vtProp.bstrVal != NULL) {
            ai.name = QString::fromWCharArray(vtProp.bstrVal, SysStringLen(vtProp.bstrVal));
        }
        VariantClear(&vtProp);

        // Get the value of the State property
        hr = pclsObj->Get(L"productState", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            if (vtProp.vt == VT_I4 || vtProp.vt == VT_UI4) {
                ai.state = vtProp.uintVal;
                ai.bStateAvailable = true;
            }
            VariantClear(&vtProp);
        }

        ai.productType = productType;

        listAv << ai;
    }
    return listAv;
}

QString InstalledAntiviruses_win::makeStrFromList(const QList<InstalledAntiviruses_win::AntivirusInfo> &other_list)
{
    QString ret = "(";
    for (int i = 0; i < other_list.count(); ++i) {
        if (other_list.at(i).bStateAvailable) {
            ret += "name = " + other_list.at(i).name + ", state = " + QString::number(other_list.at(i).state) + " [" + recognizeState(other_list.at(i).state) + "]";
        } else {
            ret += "name = " + other_list.at(i).name;
        }
        if (i != other_list.count() - 1) {
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
    if (hexValue.mid(2, 2) == "10" || hexValue.mid(2, 2) == "11") {
        ret += "enabled";
    } else if (hexValue.mid(2, 2) == "00" || hexValue.mid(2, 2) == "01") {
        ret += "disabled";
    }

    if (hexValue.mid(4, 2) == "00") {
        if (!ret.isEmpty()) {
            ret += " ";
        }
        ret += "up-to-date";
    } else if (hexValue.mid(4, 2) == "10") {
        if (!ret.isEmpty()) {
            ret += " ";
        }
        ret += "outdated";
    }

    return ret;
}
