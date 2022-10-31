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
	NT_ASSERT(inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4);
	NT_ASSERT(filter->providerContext);
	NT_ASSERT(filter->providerContext->type == FWPM_GENERAL_CONTEXT);
	NT_ASSERT(filter->providerContext->dataBuffer);
	NT_ASSERT(filter->providerContext->dataBuffer->size == sizeof(WINDSCRIBE_CALLOUT_DATA));
	NT_ASSERT(filter->providerContext->dataBuffer->data);

	NTSTATUS status = STATUS_SUCCESS;
	UINT64 classifyHandle = 0;
	FWPS_BIND_REQUEST *bindRequest = NULL;
	WINDSCRIBE_CALLOUT_DATA *calloutData = (WINDSCRIBE_CALLOUT_DATA *)filter->providerContext->dataBuffer->data;

	if (layerData && classifyContext)
	{
		if (classifyOut->rights & FWPS_RIGHT_ACTION_WRITE &&
			!(inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE))
		{
			
			status = FwpsAcquireClassifyHandle((void *)classifyContext, 0, &classifyHandle);
			if (status != STATUS_SUCCESS)
			{
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireClassifyHandle failed\n"));
				classifyHandle = 0;
				goto HLPR_BAIL_LABEL;
			}

			
			status = FwpsAcquireWritableLayerDataPointer(classifyHandle, filter->filterId, 0, &bindRequest, classifyOut);
			if (status != STATUS_SUCCESS)
			{
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: FwpsAcquireWritableLayerDataPointer failed\n"));
				bindRequest = NULL;
				goto HLPR_BAIL_LABEL;
			}

			// Prevent infinite redirection
			UINT32 timesRedirected = 0;

			for (FWPS_BIND_REQUEST* pBindRequest = bindRequest->previousVersion; pBindRequest; pBindRequest = pBindRequest->previousVersion)
			{
				if (pBindRequest->modifierFilterId == filter->filterId)
					timesRedirected++;

				/// Don't redirect the same socket more than 3 times
				if (timesRedirected > 3)
				{
					status = STATUS_TOO_MANY_COMMANDS;
					goto HLPR_BAIL_LABEL;
				}
			}

			UINT32 addr;
			RtlCopyMemory(&addr, INETADDR_ADDRESS((SOCKADDR *)&(bindRequest->localAddressAndPort)), IPV4_ADDRESS_LENGTH);
			UCHAR strIp[256];
			ULONG len = 200;
			HlprIPAddressV4ValueToString(addr, (PWSTR)strIp, &len);

			INETADDR_SET_ADDRESS((SOCKADDR *)&(bindRequest->localAddressAndPort), (const UCHAR *)&calloutData->localIp);

			UINT32 addr2;
			RtlCopyMemory(&addr2, INETADDR_ADDRESS((SOCKADDR *)&(bindRequest->localAddressAndPort)), IPV4_ADDRESS_LENGTH);
			UCHAR strIp2[256];
			ULONG len2 = 200;
			HlprIPAddressV4ValueToString(addr2, (PWSTR)strIp2, &len2);

			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: replaced %S -> %S\n", strIp, strIp2));

			classifyOut->actionType = FWP_ACTION_PERMIT;

			FwpsApplyModifiedLayerData(classifyHandle, bindRequest, FWPS_CLASSIFY_FLAG_REAUTHORIZE_IF_MODIFIED_BY_OTHERS);
			FwpsReleaseClassifyHandle(classifyHandle);


		HLPR_BAIL_LABEL:

			if (status != STATUS_SUCCESS)
			{
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "WindscribeSplitTunnel: ClassifyFn failed\n"));

				if (classifyHandle)
				{
					if (bindRequest)
					{
						FwpsApplyModifiedLayerData(classifyHandle, bindRequest, FWPS_CLASSIFY_FLAG_REAUTHORIZE_IF_MODIFIED_BY_OTHERS);
					}
					FwpsReleaseClassifyHandle(classifyHandle);
				}
				classifyOut->actionType = FWP_ACTION_BLOCK;
			}
		}
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