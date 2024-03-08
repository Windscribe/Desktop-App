#pragma once

#include <openssl/ssl.h>
#include <string>
#include <vector>

namespace wsnet {

class CertManager
{
public:
    CertManager();
    ~CertManager();

    int count();
    X509 *getCert(int ind);

private:
    struct CertDescr
    {
        X509 *cert = nullptr;
        BIO *bio = nullptr;
    };

    std::vector<CertDescr> certs_;

    void parseCertsBundle(const std::string &arr);
    CertDescr loadCert(const std::string_view &data);
    void cleanCerts();


    std::string_view sub_string(std::string_view s, std::size_t p, std::size_t n = std::string_view::npos)
    {
        return s.substr(p, n);
    }
};

} // namespace wsnet
