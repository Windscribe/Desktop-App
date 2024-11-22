#include "adaptermetricscontroller_win.h"

#include <WinSock2.h>
#include <windows.h>
#include <iphlpapi.h>

#include "engine/helper/helper_win.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

void AdapterMetricsController_win::updateMetrics(const QString &adapterName, IHelper *helper)
{
    if (adapterName.isEmpty()) {
        qCDebug(LOG_BASIC) << "AdapterMetricsController_win::updateMetrics(), Error, adapterName is empty";
        WS_ASSERT(false);
        return;
    }

    ULONG bufSize = sizeof(IP_ADAPTER_ADDRESSES_LH) * 32;
    QByteArray addresses(bufSize, Qt::Uninitialized);

    ULONG result = ::GetAdaptersAddresses(AF_UNSPEC, 0, NULL, (PIP_ADAPTER_ADDRESSES)addresses.data(), &bufSize);

    if (result == ERROR_BUFFER_OVERFLOW) {
        addresses.resize(bufSize);
        result = ::GetAdaptersAddresses(AF_UNSPEC, 0, NULL, (PIP_ADAPTER_ADDRESSES)addresses.data(), &bufSize);
    }

    if (result != NO_ERROR) {
        qCDebug(LOG_BASIC) << "AdapterMetricsController_win::updateMetrics(): GetAdaptersAddresses failed (" << result << ")";
        return;
    }

    ULONG minIPv4Metric = ULONG_MAX;
    ULONG minIPv6Metric = ULONG_MAX;
    ULONG tapAdapterIPv4Metric = ULONG_MAX;
    ULONG tapAdapterIPv6Metric = ULONG_MAX;
    bool tapAdapterIPv4Enabled = false;
    bool tapAdapterIPv6Enabled = false;
    bool bTapAdapterFound = false;
    QString tapFriendlyName;

    PIP_ADAPTER_ADDRESSES aa = (PIP_ADAPTER_ADDRESSES)addresses.data();
    while (aa) {
        QString adapterDescr = QString::fromUtf16((const ushort *)aa->Description);
        //qDebug() << "adapterName:" << adapterName << ";" << "adapterDescr:" << adapterDescr;

        if (adapterName == adapterDescr) {
            if (aa->Ipv4Enabled) {
                tapAdapterIPv4Enabled = true;
                tapAdapterIPv4Metric = aa->Ipv4Metric;
            }
            if (aa->Ipv6Enabled) {
                tapAdapterIPv6Enabled = true;
                tapAdapterIPv6Metric = aa->Ipv6Metric;
            }
            tapFriendlyName = QString::fromStdWString(aa->FriendlyName);
            if (bTapAdapterFound) {
                WS_ASSERT(false);
                qCDebug(LOG_BASIC) << "AdapterMetricsController_win::updateMetrics(), Error, two adapters with the same name found";
            }
            bTapAdapterFound = true;
        }
        else {
            if (aa->Ipv4Metric < minIPv4Metric && aa->Ipv4Enabled) {
                minIPv4Metric = aa->Ipv4Metric;
            }
            if (aa->Ipv6Metric < minIPv6Metric && aa->Ipv6Enabled) {
                minIPv6Metric = aa->Ipv6Metric;
            }
        }

        aa = aa->Next;
    }

    if (bTapAdapterFound) {
        Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
        WS_ASSERT(helper_win);

        if (tapAdapterIPv4Enabled && tapAdapterIPv4Metric >= minIPv4Metric) {
            ULONG setupIPv4Metric = minIPv4Metric;
            if (setupIPv4Metric > 2) {
                setupIPv4Metric--;
            }
            QString cmd = "netsh int ipv4 set interface interface=\"" + tapFriendlyName + "\" metric=" + QString::number(setupIPv4Metric);
            qCDebug(LOG_BASIC) << "Execute cmd:" << cmd;
            QString answer = helper_win->executeSetMetric("ipv4", tapFriendlyName, QString::number(setupIPv4Metric));
            qCDebug(LOG_BASIC) << "Answer from netsh cmd:" << answer;
        }
        if (tapAdapterIPv6Enabled && tapAdapterIPv6Metric >= minIPv6Metric) {
            ULONG setupIPv6Metric = minIPv6Metric;
            if (setupIPv6Metric > 2) {
                setupIPv6Metric--;
            }
            QString cmd = "netsh int ipv6 set interface interface=\"" + tapFriendlyName + "\" metric=" + QString::number(setupIPv6Metric);
            qCDebug(LOG_BASIC) << "Execute cmd:" << cmd;
            QString answer = helper_win->executeSetMetric("ipv6", tapFriendlyName, QString::number(setupIPv6Metric));
            qCDebug(LOG_BASIC) << "Answer from netsh cmd:" << answer;
        }
    }
    else {
        qCDebug(LOG_BASIC) << "AdapterMetricsController_win::updateMetrics(), TAP-adapter not found";
    }
}
