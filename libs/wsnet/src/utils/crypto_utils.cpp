#include "crypto_utils.h"
#include <vector>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

// ignore deprecated warnings
#ifdef _MSC_VER
    #pragma warning (disable : 4996)
#else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

std::string crypto_utils::sha1(const std::string &str)
{
    std::vector<unsigned char> message_digest(SHA_DIGEST_LENGTH);
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, str.data(), str.size());
    SHA1_Final(message_digest.data(), &ctx);
    return fmt::format("{:02x}", fmt::join(message_digest, ""));
}

std::string crypto_utils::md5(const std::string &str)
{
    std::vector<unsigned char> message_digest(MD5_DIGEST_LENGTH);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, str.data(), str.size());
    MD5_Final(message_digest.data(), &ctx);
    return fmt::format("{:02x}", fmt::join(message_digest, ""));
}

#ifdef _MSC_VER
    #pragma warning (default : 4996)
#else
    #pragma GCC diagnostic pop
#endif

