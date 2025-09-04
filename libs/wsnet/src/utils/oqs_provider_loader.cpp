#include "utils/oqs_provider_loader.h"
#include <openssl/provider.h>
#include <openssl/err.h>
#include <stdlib.h>
#include "utils/wsnet_logger.h"

extern "C" {
    extern OSSL_provider_init_fn oqs_provider_init;
}

namespace wsnet {

static const char *kOQSProviderName = "oqsprovider";
static OSSL_PROVIDER* default_provider = nullptr;
static OSSL_PROVIDER* oqs_provider = nullptr;

static int load_oqs_provider_internal(OSSL_LIB_CTX *libctx)
{
    int ret = OSSL_PROVIDER_available(libctx, kOQSProviderName);
    if (ret != 0) {
        return 0;
    }

    ret = OSSL_PROVIDER_add_builtin(libctx, kOQSProviderName, oqs_provider_init);
    if (ret != 1) {
        return -1;
    }

    OSSL_PROVIDER *provider = OSSL_PROVIDER_load(libctx, kOQSProviderName);
    if (provider == NULL) {
        return -1;
    }

    ret = OSSL_PROVIDER_self_test(provider);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

bool OQSProviderLoader::initializeOQSProvider()
{
    default_provider = OSSL_PROVIDER_load(nullptr, "default");
    if (!default_provider) {
        return false;
    }

    OSSL_LIB_CTX *global_ctx = OSSL_LIB_CTX_get0_global_default();
    if (load_oqs_provider_internal(global_ctx) == 0) {
        oqs_provider = OSSL_PROVIDER_load(nullptr, kOQSProviderName);
    }

    return true;
}

void OQSProviderLoader::cleanup()
{
    if (oqs_provider) {
        OSSL_PROVIDER_unload(oqs_provider);
        oqs_provider = nullptr;
    }

    if (default_provider) {
        OSSL_PROVIDER_unload(default_provider);
        default_provider = nullptr;
    }
}

bool OQSProviderLoader::configureSSLContext(SSL_CTX* ctx)
{
    if (!ctx) {
        return false;
    }

    const char* pq_curves = "X25519MLKEM768:p521_mlkem1024:mlkem768:p384_mlkem768:X448:X25519:secp521r1:secp384r1:secp256r1";
    if (SSL_CTX_set1_curves_list(ctx, pq_curves) != 1) {
        const char* fallback_curves = "X448:X25519:secp521r1:secp384r1:secp256r1";
        SSL_CTX_set1_curves_list(ctx, fallback_curves);
    }

    return true;
}

} // namespace wsnet