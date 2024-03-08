#include "certmanager.h"
#include <assert.h>
#include <spdlog/spdlog.h>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(wsnet);

namespace wsnet {

CertManager::CertManager()
{
    auto fs = cmrc::wsnet::get_filesystem();
    assert(fs.is_file("resources/certs_bundle.pem"));
    assert(fs.is_file("resources/windscribe_cert.crt"));

    // load certificates from bundle
    auto fdBundle = fs.open("resources/certs_bundle.pem");
    parseCertsBundle(std::string(fdBundle.begin(), fdBundle.end()));

    // load Windscribe certificates
    auto fdCert = fs.open("resources/windscribe_cert.crt");
    parseCertsBundle(std::string(fdCert.begin(), fdCert.end()));

    spdlog::info("CertManager number of certificates : {}", certs_.size());
}

CertManager::~CertManager()
{
    cleanCerts();
}

int CertManager::count()
{
    return (int)certs_.size();
}

X509 *CertManager::getCert(int ind)
{
    return certs_[ind].cert;
}

void CertManager::parseCertsBundle(const std::string &arr)
{
    size_t curOffs = 0;
    std::string beginSample("-----BEGIN CERTIFICATE-----");
    std::string endSample("-----END CERTIFICATE-----");
    size_t endSampleSize = endSample.size();

    while (true) {
        size_t indStart = arr.find(beginSample, curOffs);
        size_t indEnd = arr.find(endSample, indStart);

        if (indStart != std::string::npos && indEnd != std::string::npos) {
            std::string_view cert = sub_string(arr, indStart, indEnd + endSampleSize);
            certs_.push_back(loadCert(cert));
            curOffs = indEnd + endSampleSize;
        } else {
            break;
        }
    }
}

CertManager::CertDescr CertManager::loadCert(const std::string_view &data)
{
    CertDescr cd;
    const char *mypem = data.data();
    cd.bio = BIO_new_mem_buf(mypem, -1);
    PEM_read_bio_X509(cd.bio, &cd.cert, 0, NULL);
    if(cd.cert == NULL) {
        assert(false);
    }
    return cd;
}

void CertManager::cleanCerts()
{
    for (const auto & it : certs_) {
        X509_free(it.cert);
        BIO_free(it.bio);
    }
    certs_.clear();
}

} // namespace wsnet
