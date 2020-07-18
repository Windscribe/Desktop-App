#include "detectlocalip.h"
#include "utils/logger.h"

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <iphlpapi.h>
#include <QHostAddress>
#endif

QString DetectLocalIP::getLocalIP()
{
#ifdef Q_OS_MAC
    //QString primaryInterface = macGetPrimaryInterface();
    //return macGetIpForInterface(primaryInterface);
    return macGetLocalIP();
#elif defined Q_OS_WIN

    ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
    std::vector<unsigned char> pAdapterInfo(ulAdapterInfoSize);

    if (GetAdaptersInfo((IP_ADAPTER_INFO *)&pAdapterInfo[0], &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW) // out of buff
    {
        pAdapterInfo.resize(ulAdapterInfoSize);
    }

    if (GetAdaptersInfo((IP_ADAPTER_INFO *)&pAdapterInfo[0], &ulAdapterInfoSize) == ERROR_SUCCESS)
    {
        IP_ADAPTER_INFO *ai = (IP_ADAPTER_INFO *)&pAdapterInfo[0];

        do
        {
            if ((ai->Type == MIB_IF_TYPE_ETHERNET) 	// If type is etherent
                || (ai->Type == IF_TYPE_IEEE80211))   // radio
            {
                if (strstr(ai->Description, "Windscribe VPN") == 0 && strcmp(ai->IpAddressList.IpAddress.String, "0.0.0.0") != 0
                        && strcmp(ai->GatewayList.IpAddress.String, "0.0.0.0") != 0)
                {
                    return ai->IpAddressList.IpAddress.String;
                }
            }
            ai = ai->Next;
        } while (ai);
    }

#endif
    return "";
}

#ifdef Q_OS_MAC
QString DetectLocalIP::macGetPrimaryInterface()
{
    char strReply[4096];
    strReply[0] = '\0';
    FILE *file = popen("echo 'show State:/Network/Global/IPv4' | scutil | grep PrimaryInterface | sed -e 's/.*PrimaryInterface : //'", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strcat(strReply, szLine);
        }
        pclose(file);
    }

    QString primaryInterface = QString::fromLocal8Bit(strReply).trimmed();
    if (primaryInterface.isEmpty())
    {
        qCDebug(LOG_BASIC) << "no primary interface in DetectLocalIP::macGetPrimaryInterface()";
        return "";
    }
    return primaryInterface;
}

QString DetectLocalIP::macGetIpForInterface(const QString &interface)
{
    QString cmd = "ipconfig getifaddr " + interface;
    char strReply[4096];
    strReply[0] = '\0';
    FILE *file = popen(cmd.toStdString().c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strcat(strReply, szLine);
        }
        pclose(file);
    }
    return QString::fromLocal8Bit(strReply).trimmed();
}

QString DetectLocalIP::macGetLocalIP()
{
    char strReply[4096];
    strReply[0] = '\0';
    FILE *file = popen("ifconfig | grep -E \"([0-9]{1,3}\\.){3}[0-9]{1,3}\" | grep -v 127.0.0.1 | awk '{ print $2 }' | cut -f2 -d: | head -n1", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strcat(strReply, szLine);
        }
        pclose(file);
    }

    QString ip = QString::fromLocal8Bit(strReply).trimmed();
    return ip;
}
#endif
