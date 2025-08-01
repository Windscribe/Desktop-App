#pragma once

#include <openssl/ssl.h>

namespace wsnet {

class OQSProviderLoader {
public:
    static bool initializeOQSProvider();
    static void cleanup();
    static bool configureSSLContext(SSL_CTX* ctx);
};

} // namespace wsnet