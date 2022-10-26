#include <ntddk.h>
#pragma warning(disable : 4201)
#include <fwpsk.h>
#include <mstcpip.h>
#include <fwpmtypes.h>

#include "CalloutFunctions.h"
#include "WindscribeCallout.h"

#define IPV4_ADDRESS_LENGTH  4


void HlprIPAddressV4ValueToString(UINT32 pIPv4Address, _Inout_ PWSTR pIPv4AddressString, _Inout_ ULONG *addressStringLength)
{
    RtlIpv4AddressToStringExW((const IN_ADDR *)&pIPv4Address, 0, pIPv4AddressString, addressStringLength);
}

static int isLanAddress(UINT32 ip)
{
    // 10.255.255.0/24 is reserved and not considered a LAN address
    if ((ip & 0xFFFFFF) == 0xFFFF0A) {
        return 0;
    }

    return ((ip & 0xFFFF) == 0xA8C0 || // 192.168.0.0/16
            (ip & 0xF0FF) == 0x10AC || // 172.16.0.0/12
            (ip & 0xFFFF) == 0xFEA9 || // 169.254.0.0/16
            (ip & 0xFF) == 0x0A ||     // 10.0.0.0/8
            (ip & 0xFF) == 0x7F ||     // 127.0.0.0/8
            (ip & 0xF0) == 0xE0);      // 224.0.0.0/4
}

void redirect(SOCKADDR *address, UINT32 newAddress)
{
    UINT32 addr;
    RtlCopyMemory(&addr, INETADDR_ADDRESS(address), IPV4_ADDRESS_LENGTH);
    UCHAR strIp[16];
    ULONG len = 0;
    HlprIPAddressV4ValueToString(addr, (PWSTR)strIp, &len);

    INETADDR_SET_ADDRESS(address, (const UCHAR *)&newAddress);

    UINT32 addr2;
    RtlCopyMemory(&addr2, INETADDR_ADDRESS(address), IPV4_ADDRESS_LENGTH);
    UCHAR strIp2[16];
    ULONG len2 = 0;
    HlprIPAddressV4ValueToString(addr2, (PWSTR)strIp2, &len2);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: replaced %S -> %S\n", strIp, strIp2));
}

VOID NTAPI
ClassifyFn(
    IN const FWPS_INCOMING_VALUES0* inFixedValues,
    IN const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    IN OUT VOID* layerData,
    IN const void* classifyContext,
    IN const FWPS_FILTER1* filter,
    IN UINT64  flowContext,
    IN OUT FWPS_CLASSIFY_OUT0* classifyOut
)
{
    UNREFERENCED_PARAMETER(inMetaValues);
    UNREFERENCED_PARAMETER(flowContext);

    NT_ASSERT(inFixedValues);
    NT_ASSERT(inMetaValues);
    NT_ASSERT(layerData);
    NT_ASSERT(classifyContext);
    NT_ASSERT(filter);
    NT_ASSERT(classifyOut);
    NT_ASSERT(filter->providerContext);
    NT_ASSERT(filter->providerContext->type == FWPM_GENERAL_CONTEXT);
    NT_ASSERT(filter->providerContext->dataBuffer);
    NT_ASSERT(filter->providerContext->dataBuffer->size == sizeof(WINDSCRIBE_CALLOUT_DATA));
    NT_ASSERT(filter->providerContext->dataBuffer->data);

    NTSTATUS status = STATUS_SUCCESS;
    UINT64 classifyHandle = 0;
    PVOID dataPointer = NULL;
    FWPS_BIND_REQUEST *bindRequest = NULL;
    FWPS_CONNECT_REQUEST *connectRequest = NULL;
    WINDSCRIBE_CALLOUT_DATA *calloutData = (WINDSCRIBE_CALLOUT_DATA *)filter->providerContext->dataBuffer->data;

    if (!layerData || !classifyContext || !(classifyOut->rights & FWPS_RIGHT_ACTION_WRITE)) {
        return;
    }

    status = FwpsAcquireClassifyHandle((void *)classifyContext, 0, &classifyHandle);
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireClassifyHandle failed\n"));
        classifyHandle = 0;
        goto error;
    }

    status = FwpsAcquireWritableLayerDataPointer(classifyHandle, filter->filterId, 0, &dataPointer, classifyOut);
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireWritableLayerDataPointer failed\n"));
        dataPointer = NULL;
        goto error;
    }

    if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4) {
        connectRequest = (FWPS_CONNECT_REQUEST *)dataPointer;

        // Prevent infinite redirection
        UINT32 timesRedirected = 0;
        for (FWPS_CONNECT_REQUEST* pConnectRequest = connectRequest->previousVersion; pConnectRequest; pConnectRequest = pConnectRequest->previousVersion) {
            if (pConnectRequest->modifierFilterId == filter->filterId) {
                timesRedirected++;
            }

            // Don't redirect the same socket more than 3 times
            if (timesRedirected > 3) {
                status = STATUS_TOO_MANY_COMMANDS;
                goto error;
            }
        }

        UINT32 remoteAddr = 0;
        RtlCopyMemory(&remoteAddr, INETADDR_ADDRESS((SOCKADDR *)&(connectRequest->remoteAddressAndPort)), IPV4_ADDRESS_LENGTH);

        if (!calloutData->isExclude && calloutData->isAllowLanTraffic && isLanAddress(remoteAddr))
        {
            // If inclusive mode and allowing LAN traffic and remote is LAN, don't redirect
            classifyOut->actionType = FWP_ACTION_PERMIT;
            goto cleanup;
        }

        redirect((SOCKADDR *)&(connectRequest->localAddressAndPort), calloutData->localIp);

    } else if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4) {
        if (inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE) {
            goto cleanup;
        }

        bindRequest = (FWPS_BIND_REQUEST *)dataPointer;

        // Prevent infinite redirection
        UINT32 timesRedirected = 0;

        for (FWPS_BIND_REQUEST* pBindRequest = bindRequest->previousVersion; pBindRequest; pBindRequest = pBindRequest->previousVersion) {
            if (pBindRequest->modifierFilterId == filter->filterId) {
                timesRedirected++;
            }

            // Don't redirect the same socket more than 3 times
            if (timesRedirected > 3) {
                status = STATUS_TOO_MANY_COMMANDS;
                goto error;
            }
        }

        redirect((SOCKADDR *)&(bindRequest->localAddressAndPort), calloutData->localIp);
    }

error:
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: ClassifyFn failed\n"));
        classifyOut->actionType = FWP_ACTION_BLOCK;
    } else {
        classifyOut->actionType = FWP_ACTION_PERMIT;
    }

cleanup:
    if (classifyHandle) {
        if (dataPointer) {
            FwpsApplyModifiedLayerData(classifyHandle, dataPointer, FWPS_CLASSIFY_FLAG_REAUTHORIZE_IF_MODIFIED_BY_OTHERS);
        }
        FwpsReleaseClassifyHandle(classifyHandle);
    }
}

NTSTATUS NTAPI
NotifyFn(
    IN FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    IN const GUID* filterKey,
    IN const FWPS_FILTER1* filter
)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    if (notifyType == FWPS_CALLOUT_NOTIFY_ADD_FILTER)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: NotifyFn FWPS_CALLOUT_NOTIFY_ADD_FILTER\n"));
    }
    else if (notifyType == FWPS_CALLOUT_NOTIFY_DELETE_FILTER)
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: NotifyFn FWPS_CALLOUT_NOTIFY_DELETE_FILTER\n"));
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: NotifyFn called\n"));
    }
    return STATUS_SUCCESS;
}