#include <ntddk.h>
#pragma warning(disable : 4201)
#include <fwpsk.h>
#include <mstcpip.h>
#include <fwpmtypes.h>

#include "CalloutFunctions.h"
#include "WindscribeCallout.h"

#define IPV4_ADDRESS_LENGTH  4
#define IPV6_ADDRESS_LENGTH  16

// Set to 1 to enable verbose address-replacement logging (adds overhead per packet)
#define WS_CALLOUT_DEBUG 0

#if WS_CALLOUT_DEBUG

void HlprIPAddressV4ValueToString(UINT32 pIPv4Address, _Inout_ PWSTR pIPv4AddressString, _Inout_ ULONG* addressStringLength, USHORT port)
{
    RtlIpv4AddressToStringExW((const IN_ADDR*)&pIPv4Address, port, pIPv4AddressString, addressStringLength);
}

void SockaddrToString(SOCKADDR* address, wchar_t* str)
{
    ULONG len = INET_ADDRSTRLEN;
    SOCKADDR_IN* sin = (SOCKADDR_IN*)address;
    UINT32 addr;
    RtlCopyMemory(&addr, INETADDR_ADDRESS(address), IPV4_ADDRESS_LENGTH);
    HlprIPAddressV4ValueToString(addr, (PWSTR)str, &len, sin->sin_port);
}

#endif

void redirect(SOCKADDR* address, UINT32 newAddress)
{
#if WS_CALLOUT_DEBUG
    wchar_t strIp[INET_ADDRSTRLEN];
    SockaddrToString(address, strIp);
#endif

    INETADDR_SET_ADDRESS(address, (const UCHAR*)&newAddress);

#if WS_CALLOUT_DEBUG
    wchar_t strIp2[INET_ADDRSTRLEN];
    SockaddrToString(address, strIp2);
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel2: replaced %S -> %S\n", strIp, strIp2));
#endif
}

void redirectV6(SOCKADDR* address, const UINT8* newAddress)
{
#if WS_CALLOUT_DEBUG
    SOCKADDR_IN6* sin6 = (SOCKADDR_IN6*)address;
    wchar_t strIp[INET6_ADDRSTRLEN];
    ULONG len = INET6_ADDRSTRLEN;
    RtlIpv6AddressToStringExW(&sin6->sin6_addr, sin6->sin6_scope_id, sin6->sin6_port, strIp, &len);
#endif

    INETADDR_SET_ADDRESS(address, newAddress);

#if WS_CALLOUT_DEBUG
    wchar_t strIp2[INET6_ADDRSTRLEN];
    len = INET6_ADDRSTRLEN;
    RtlIpv6AddressToStringExW(&sin6->sin6_addr, sin6->sin6_scope_id, sin6->sin6_port, strIp2, &len);
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel2: replaced v6 %S -> %S\n", strIp, strIp2));
#endif
}

static BOOLEAN isV6AddressInExclusionList(const UINT8 *remoteAddr, const WINDSCRIBE_CALLOUT_DATA_V6 *calloutData)
{
    int i, j;
    for (i = 0; i < calloutData->cntExcludeAddresses / 2; i++) {
        const UINT8 *addr = &calloutData->excludeAddresses[i * 2 * IPV6_ADDRESS_LENGTH];
        const UINT8 *mask = &calloutData->excludeAddresses[(i * 2 + 1) * IPV6_ADDRESS_LENGTH];
        BOOLEAN match = TRUE;
        for (j = 0; j < IPV6_ADDRESS_LENGTH; j++) {
            if ((remoteAddr[j] & mask[j]) != addr[j]) {
                match = FALSE;
                break;
            }
        }
        if (match)
            return TRUE;
    }
    return FALSE;
}

// Anyone able to add WFP filters can point one at our callout GUIDs with no provider context
// at all, so nothing about the blob may be assumed. Returns NULL unless the context is present
// and large enough for the address list its own cntExcludeAddresses claims.
static const FWP_BYTE_BLOB *getCalloutDataBlob(const FWPS_FILTER1 *filter, SIZE_T headerSize)
{
    const FWP_BYTE_BLOB *blob;

    if (!filter->providerContext || filter->providerContext->type != FWPM_GENERAL_CONTEXT) {
        return NULL;
    }

    blob = filter->providerContext->dataBuffer;
    if (!blob || !blob->data || blob->size < headerSize) {
        return NULL;
    }
    return blob;
}

static WINDSCRIBE_CALLOUT_DATA *getCalloutData(const FWPS_FILTER1 *filter)
{
    const SIZE_T headerSize = FIELD_OFFSET(WINDSCRIBE_CALLOUT_DATA, excludeAddresses);
    const FWP_BYTE_BLOB *blob = getCalloutDataBlob(filter, headerSize);
    if (!blob) {
        return NULL;
    }

    if (blob->size < headerSize +
        (SIZE_T)((const WINDSCRIBE_CALLOUT_DATA *)blob->data)->cntExcludeAddresses * sizeof(UINT32)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: callout data too small\n"));
        return NULL;
    }
    return (WINDSCRIBE_CALLOUT_DATA *)blob->data;
}

static WINDSCRIBE_CALLOUT_DATA_V6 *getCalloutDataV6(const FWPS_FILTER1 *filter)
{
    const SIZE_T headerSize = FIELD_OFFSET(WINDSCRIBE_CALLOUT_DATA_V6, excludeAddresses);
    const FWP_BYTE_BLOB *blob = getCalloutDataBlob(filter, headerSize);
    if (!blob) {
        return NULL;
    }

    if (blob->size < headerSize +
        (SIZE_T)((const WINDSCRIBE_CALLOUT_DATA_V6 *)blob->data)->cntExcludeAddresses * IPV6_ADDRESS_LENGTH) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: callout data (v6) too small\n"));
        return NULL;
    }
    return (WINDSCRIBE_CALLOUT_DATA_V6 *)blob->data;
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
    NT_ASSERT(filter);
    NT_ASSERT(classifyOut);

    NTSTATUS status = STATUS_SUCCESS;
    UINT64 classifyHandle = 0;
    PVOID dataPointer = NULL;
    FWPS_BIND_REQUEST *bindRequest = NULL;
    FWPS_CONNECT_REQUEST *connectRequest = NULL;
    WINDSCRIBE_CALLOUT_DATA *calloutData = getCalloutData(filter);

    if (!calloutData || !layerData || !classifyContext || !(classifyOut->rights & FWPS_RIGHT_ACTION_WRITE)) {
        return;
    }

    status = FwpsAcquireClassifyHandle((void*)classifyContext, 0, &classifyHandle);
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireClassifyHandle failed\n"));
        classifyHandle = 0;
        goto error;
    }

    if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4) {
        // The socket is already bound to our address on a reauthorize pass, and re-entering
        // the modify path here is what lets a second FwpsApplyModifiedLayerData reach the
        // same layer data (bugchecks NETIO!FeApplyModifiedLayerData).
        if (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE) {
            goto cleanup;
        }

        // skip modification if a remote address in the exclusion list(these are usually local address ranges)
        UINT32 remoteIp = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_ADDRESS].value.uint32;
        for (int i = 0; i < calloutData->cntExcludeAddresses / 2; i++) {
            if ((remoteIp & calloutData->excludeAddresses[i * 2 + 1]) == calloutData->excludeAddresses[i * 2]) {  // in local range?
                KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: skipped: %X\n", remoteIp));
                goto cleanup;
            }
        }
    } else if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4) {
        if (inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE) {
            goto cleanup;
        }
    }

    // All "skip" checks must run before this point: a successful acquire obliges us to call
    // FwpsApplyModifiedLayerData, and that may be done only once per acquired layer data.
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

        if (calloutData->isExclude) {
            redirect((SOCKADDR*)&(connectRequest->localAddressAndPort), calloutData->localIp);
        }
        else {
            redirect((SOCKADDR*)&(connectRequest->localAddressAndPort), calloutData->vpnIp);
        }

    } else if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4) {
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

        if (calloutData->isExclude) {
            redirect((SOCKADDR *)&(bindRequest->localAddressAndPort), calloutData->localIp);
        } else {
            redirect((SOCKADDR *)&(bindRequest->localAddressAndPort), calloutData->vpnIp);
        }
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

VOID NTAPI
ClassifyFnV6(
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
    NT_ASSERT(filter);
    NT_ASSERT(classifyOut);

    NTSTATUS status = STATUS_SUCCESS;
    UINT64 classifyHandle = 0;
    PVOID dataPointer = NULL;
    FWPS_BIND_REQUEST *bindRequest = NULL;
    FWPS_CONNECT_REQUEST *connectRequest = NULL;
    WINDSCRIBE_CALLOUT_DATA_V6 *calloutData = getCalloutDataV6(filter);

    if (!calloutData || !layerData || !classifyContext || !(classifyOut->rights & FWPS_RIGHT_ACTION_WRITE)) {
        return;
    }

    status = FwpsAcquireClassifyHandle((void*)classifyContext, 0, &classifyHandle);
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireClassifyHandle (v6) failed\n"));
        classifyHandle = 0;
        goto error;
    }

    if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6) {
        // See the v4 path: skip reauthorize before entering the modify path.
        if (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE) {
            goto cleanup;
        }

        FWP_BYTE_ARRAY16 *remoteIp = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_REMOTE_ADDRESS].value.byteArray16;
        if (isV6AddressInExclusionList(remoteIp->byteArray16, calloutData)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: skipped v6 (local range)\n"));
            goto cleanup;
        }
    } else if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6) {
        if (inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V6_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE) {
            goto cleanup;
        }
    }

    // All "skip" checks must run before this point: a successful acquire obliges us to call
    // FwpsApplyModifiedLayerData, and that may be done only once per acquired layer data.
    status = FwpsAcquireWritableLayerDataPointer(classifyHandle, filter->filterId, 0, &dataPointer, classifyOut);
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireWritableLayerDataPointer (v6) failed\n"));
        dataPointer = NULL;
        goto error;
    }

    if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6) {
        connectRequest = (FWPS_CONNECT_REQUEST *)dataPointer;

        UINT32 timesRedirected = 0;
        for (FWPS_CONNECT_REQUEST* pConnectRequest = connectRequest->previousVersion; pConnectRequest; pConnectRequest = pConnectRequest->previousVersion) {
            if (pConnectRequest->modifierFilterId == filter->filterId)
                timesRedirected++;
            if (timesRedirected > 3) {
                status = STATUS_TOO_MANY_COMMANDS;
                goto error;
            }
        }

        if (calloutData->isExclude) {
            redirectV6((SOCKADDR*)&(connectRequest->localAddressAndPort), calloutData->localIp);
        } else {
            redirectV6((SOCKADDR*)&(connectRequest->localAddressAndPort), calloutData->vpnIp);
        }

    } else if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6) {
        bindRequest = (FWPS_BIND_REQUEST *)dataPointer;

        UINT32 timesRedirected = 0;
        for (FWPS_BIND_REQUEST* pBindRequest = bindRequest->previousVersion; pBindRequest; pBindRequest = pBindRequest->previousVersion) {
            if (pBindRequest->modifierFilterId == filter->filterId)
                timesRedirected++;
            if (timesRedirected > 3) {
                status = STATUS_TOO_MANY_COMMANDS;
                goto error;
            }
        }

        if (calloutData->isExclude) {
            redirectV6((SOCKADDR *)&(bindRequest->localAddressAndPort), calloutData->localIp);
        } else {
            redirectV6((SOCKADDR *)&(bindRequest->localAddressAndPort), calloutData->vpnIp);
        }
    }

error:
    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: ClassifyFnV6 failed\n"));
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