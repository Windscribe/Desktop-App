#include <ntddk.h>
#pragma warning(disable : 4201)
#include <fwpsk.h>
#include <mstcpip.h>
#include <fwpmtypes.h>

#include "CalloutFunctions.h"
#include "WindscribeCallout.h"

#define IPV4_ADDRESS_LENGTH  4

void HlprIPAddressV4ValueToString(UINT32 pIPv4Address, _Inout_ PWSTR pIPv4AddressString, _Inout_ ULONG *addressStringLength, USHORT port)
{
    RtlIpv4AddressToStringExW((const IN_ADDR *)&pIPv4Address, port, pIPv4AddressString, addressStringLength);
}

void SockaddrToString(SOCKADDR* address, wchar_t* str)
{
    ULONG len = INET_ADDRSTRLEN;
    SOCKADDR_IN* sin = (SOCKADDR_IN* )address;
    UINT32 addr;
    RtlCopyMemory(&addr, INETADDR_ADDRESS(address), IPV4_ADDRESS_LENGTH);
    HlprIPAddressV4ValueToString(addr, (PWSTR)str, &len, sin->sin_port);
}

void redirect(SOCKADDR *address, UINT32 newAddress)
{
    UINT32 addr;
    RtlCopyMemory(&addr, INETADDR_ADDRESS(address), IPV4_ADDRESS_LENGTH);
    wchar_t strIp[INET_ADDRSTRLEN];
    SockaddrToString(address, strIp);

    INETADDR_SET_ADDRESS(address, (const UCHAR *)&newAddress);

    UINT32 addr2;
    RtlCopyMemory(&addr2, INETADDR_ADDRESS(address), IPV4_ADDRESS_LENGTH);
    wchar_t strIp2[INET_ADDRSTRLEN];
    SockaddrToString(address, strIp2);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel2: replaced %S -> %S\n", strIp, strIp2));
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
    NT_ASSERT(filter->providerContext->dataBuffer->data);

    NTSTATUS status = STATUS_SUCCESS;
    UINT64 classifyHandle = 0;
    PVOID dataPointer = NULL;
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

    if (inFixedValues->layerId != FWPS_LAYER_ALE_CONNECT_REDIRECT_V4)
        goto cleanup;

    // skip modification if a remote address in the exclusion list(these are usually local address ranges)
    UINT32 remoteIp = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_ADDRESS].value.uint32;
    for (int i = 0; i < calloutData->cntExcludeAddresses / 2; i++) {
        if ((remoteIp & calloutData->excludeAddresses[i*2 + 1]) == calloutData->excludeAddresses[i*2]) {  // in local range?
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: skipped: %X\n", remoteIp));
            goto cleanup;
        }
    }

    status = FwpsAcquireWritableLayerDataPointer(classifyHandle, filter->filterId, 0, &dataPointer, classifyOut);
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireWritableLayerDataPointer failed\n"));
        dataPointer = NULL;
        goto error;
    }

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

    if (calloutData->isExclude) {
        redirect((SOCKADDR*)&(connectRequest->localAddressAndPort), calloutData->localIp);
    }
    else {
        redirect((SOCKADDR*)&(connectRequest->localAddressAndPort), calloutData->vpnIp);
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

    if (notifyType == FWPS_CALLOUT_NOTIFY_ADD_FILTER) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: NotifyFn FWPS_CALLOUT_NOTIFY_ADD_FILTER\n"));
    } else if (notifyType == FWPS_CALLOUT_NOTIFY_DELETE_FILTER) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: NotifyFn FWPS_CALLOUT_NOTIFY_DELETE_FILTER\n"));
    } else {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: NotifyFn called\n"));
    }
    return STATUS_SUCCESS;
}