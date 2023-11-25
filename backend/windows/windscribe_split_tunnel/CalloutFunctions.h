#pragma once

// Prototypes for the callout functions
VOID NTAPI ClassifyFn(
    IN const FWPS_INCOMING_VALUES0* inFixedValues,
    IN const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    IN OUT VOID* layerData,
    IN const void* classifyContext,
    IN const FWPS_FILTER1* filter,
    IN UINT64  flowContext,
    IN OUT FWPS_CLASSIFY_OUT0* classifyOut
);

NTSTATUS NTAPI NotifyFn(
    IN FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    IN const GUID* filterKey,
    IN const FWPS_FILTER1* filter
);

