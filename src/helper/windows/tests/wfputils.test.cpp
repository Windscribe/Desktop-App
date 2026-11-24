// Exercises the WFP provider tagging and sublayer deletion in wfp_utils.cpp against the real Base
// Filtering Engine. Requires elevation; exits 77 (skip) when run from a non-elevated shell.
//
// Safety: the engine session is dynamic (FWPM_SESSION_FLAG_DYNAMIC), so BFE deletes everything this
// test creates when the process exits, even if it crashes. It only adds PERMIT filters for TEST-NET
// addresses (192.0.2.0/24, 2001:db8::/32) in a sublayer of its own, so networking is unaffected.

#include <winsock2.h>
#include <windows.h>
#include <fwpmu.h>
#include <rpc.h>

#include <cstdio>
#include <vector>

#include "wfp_utils.h"

namespace
{

int g_failures = 0;

bool check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
    return ok;
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

bool isElevated()
{
    HANDLE token;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_ELEVATION elevation = {};
    DWORD size = sizeof(elevation);
    BOOL ok = ::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
    ::CloseHandle(token);
    return ok && elevation.TokenIsElevated;
}

GUID newGuid()
{
    GUID g;
    ::UuidCreate(&g);
    return g;
}

bool sublayerExists(HANDLE engine, const GUID &key)
{
    FWPM_SUBLAYER0 *subLayer = nullptr;
    const DWORD ret = FwpmSubLayerGetByKey0(engine, &key, &subLayer);
    if (subLayer) {
        FwpmFreeMemory0((void **)&subLayer);
    }
    return ret == ERROR_SUCCESS;
}

bool addSublayer(HANDLE engine, const GUID &key)
{
    FWPM_SUBLAYER0 subLayer = {};
    subLayer.subLayerKey = key;
    subLayer.displayData.name = (wchar_t *)L"wfputils.test sublayer";
    subLayer.weight = 0x100;
    return FwpmSubLayerAdd0(engine, &subLayer, NULL) == ERROR_SUCCESS;
}

// Returns the filter's providerKey (nullptr if untagged); ok is false if the filter is missing.
bool filterProvider(HANDLE engine, UINT64 id, bool &tagged)
{
    FWPM_FILTER0 *filter = nullptr;
    if (FwpmFilterGetById0(engine, id, &filter) != ERROR_SUCCESS) {
        return false;
    }
    tagged = filter->providerKey != nullptr && *filter->providerKey == Utils::kWfpProviderGuid;
    FwpmFreeMemory0((void **)&filter);
    return true;
}

bool filterExists(HANDLE engine, UINT64 id)
{
    FWPM_FILTER0 *filter = nullptr;
    const DWORD ret = FwpmFilterGetById0(engine, id, &filter);
    if (filter) {
        FwpmFreeMemory0((void **)&filter);
    }
    return ret == ERROR_SUCCESS;
}

// Adds the test filters with the production helpers: v4 and v6 TEST-NET permits (2 layers each)
// and an ICMPv6 type-range permit (2 layers). All non-persistent: a dynamic session cannot add
// persistent objects.
std::vector<UINT64> addTaggedFilters(HANDLE engine, const GUID &sublayer)
{
    std::vector<UINT64> ids;
    wchar_t *name = (wchar_t *)L"wfputils.test filter";
    const std::vector<types::IpAddressRange> v4 = { types::IpAddressRange("192.0.2.1/32"), types::IpAddressRange("192.0.2.2/32") };
    const std::vector<types::IpAddressRange> v6 = { types::IpAddressRange("2001:db8::1/128") };
    VERIFY(Utils::addFilterV4(engine, &ids, FWP_ACTION_PERMIT, 1, sublayer, name, nullptr, &v4, 0, 0, nullptr, false));
    VERIFY(Utils::addFilterV6(engine, &ids, FWP_ACTION_PERMIT, 1, sublayer, name, nullptr, &v6, 0, 0, nullptr, false));
    VERIFY(ids.size() == 4);
    return ids;
}

UINT64 addUntaggedFilter(HANDLE engine, const GUID &sublayer)
{
    FWP_V4_ADDR_AND_MASK addr = { (192u << 24) | (2u << 8) | 3u, 0xFFFFFFFF };
    FWPM_FILTER_CONDITION0 condition = {};
    condition.fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
    condition.matchType = FWP_MATCH_EQUAL;
    condition.conditionValue.type = FWP_V4_ADDR_MASK;
    condition.conditionValue.v4AddrMask = &addr;

    FWPM_FILTER0 filter = {};
    filter.subLayerKey = sublayer;
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    filter.displayData.name = (wchar_t *)L"wfputils.test untagged filter";
    filter.action.type = FWP_ACTION_PERMIT;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = 1;
    filter.numFilterConditions = 1;
    filter.filterCondition = &condition;
    UINT64 id = 0;
    VERIFY(FwpmFilterAdd0(engine, &filter, NULL, &id) == ERROR_SUCCESS);
    return id;
}

// Runs deleteSublayerAndAllFilters inside a committed transaction, as the firewall classes do.
bool deleteInTransaction(HANDLE engine, const GUID &sublayer)
{
    VERIFY(FwpmTransactionBegin0(engine, 0) == ERROR_SUCCESS);
    const bool ret = Utils::deleteSublayerAndAllFilters(engine, &sublayer);
    VERIFY(FwpmTransactionCommit0(engine) == ERROR_SUCCESS);
    return ret;
}

void testTaggedFiltersAreDeleted(HANDLE engine)
{
    printf("testTaggedFiltersAreDeleted\n");
    const GUID sublayer = newGuid();
    VERIFY(addSublayer(engine, sublayer));
    const std::vector<UINT64> ids = addTaggedFilters(engine, sublayer);
    for (UINT64 id : ids) {
        bool tagged = false;
        VERIFY(filterProvider(engine, id, tagged));
        VERIFY(tagged);
    }

    VERIFY(deleteInTransaction(engine, sublayer));
    VERIFY(!sublayerExists(engine, sublayer));
    for (UINT64 id : ids) {
        VERIFY(!filterExists(engine, id));
    }
}

void testUntaggedFilterFallsBackToFullScan(HANDLE engine)
{
    printf("testUntaggedFilterFallsBackToFullScan\n");
    const GUID sublayer = newGuid();
    VERIFY(addSublayer(engine, sublayer));
    std::vector<UINT64> ids = addTaggedFilters(engine, sublayer);
    ids.push_back(addUntaggedFilter(engine, sublayer));

    // The tagged pass leaves the untagged filter, the sublayer delete reports FWP_E_IN_USE, and the
    // full scan must then remove it.
    VERIFY(deleteInTransaction(engine, sublayer));
    VERIFY(!sublayerExists(engine, sublayer));
    for (UINT64 id : ids) {
        VERIFY(!filterExists(engine, id));
    }
}

void testAbsentSublayer(HANDLE engine)
{
    printf("testAbsentSublayer\n");
    const GUID sublayer = newGuid();
    VERIFY(deleteInTransaction(engine, sublayer));
}

// Only possible when this test created the provider itself (in the dynamic session); an installed
// helper's persistent provider must not be deleted.
void testMissingProviderAddsUntagged(HANDLE engine)
{
    printf("testMissingProviderAddsUntagged\n");
    VERIFY(FwpmProviderDeleteByKey0(engine, &Utils::kWfpProviderGuid) == ERROR_SUCCESS);

    const GUID sublayer = newGuid();
    VERIFY(addSublayer(engine, sublayer));
    // The provider is still believed available, so the first add hits FWP_E_PROVIDER_NOT_FOUND and
    // must retry untagged rather than fail.
    const std::vector<UINT64> ids = addTaggedFilters(engine, sublayer);
    for (UINT64 id : ids) {
        bool tagged = true;
        VERIFY(filterProvider(engine, id, tagged));
        VERIFY(!tagged);
    }

    // Deletion now has no provider and must use the full scan.
    VERIFY(deleteInTransaction(engine, sublayer));
    VERIFY(!sublayerExists(engine, sublayer));
}

} // namespace

int main()
{
    if (!isElevated()) {
        printf("SKIP: requires elevation\n");
        return 77;
    }

    FWPM_SESSION0 session = {};
    session.flags = FWPM_SESSION_FLAG_DYNAMIC;
    session.displayData.name = (wchar_t *)L"wfputils.test";
    HANDLE engine = nullptr;
    if (FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &engine) != ERROR_SUCCESS) {
        printf("FAIL: FwpmEngineOpen0\n");
        return 1;
    }

    // An installed helper has already added the persistent provider. Otherwise add a dynamic one
    // here: ensureWfpProvider adds a persistent provider, which a dynamic session may not create.
    FWPM_PROVIDER0 provider = {};
    provider.providerKey = Utils::kWfpProviderGuid;
    provider.displayData.name = (wchar_t *)L"wfputils.test provider";
    const DWORD providerRet = FwpmProviderAdd0(engine, &provider, NULL);
    const bool ownsProvider = (providerRet == ERROR_SUCCESS);
    VERIFY(ownsProvider || providerRet == FWP_E_ALREADY_EXISTS);
    VERIFY(Utils::ensureWfpProvider(engine));

    testTaggedFiltersAreDeleted(engine);
    testUntaggedFilterFallsBackToFullScan(engine);
    testAbsentSublayer(engine);
    if (ownsProvider) {
        testMissingProviderAddsUntagged(engine);
    } else {
        printf("testMissingProviderAddsUntagged skipped: the provider belongs to an installed helper\n");
    }

    FwpmEngineClose0(engine);

    if (g_failures == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("%d failure(s)\n", g_failures);
    return 1;
}
