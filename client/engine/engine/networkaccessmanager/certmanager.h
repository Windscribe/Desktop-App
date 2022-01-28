#ifndef CERTMANAGER_H
#define CERTMANAGER_H

#include <QByteArray>
#include <QStringList>
#include <QVector>
#include <openssl/ssl.h>

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
        X509 *cert;
        BIO *bio;
    };

    void parseCertsBundle(QByteArray &arr);
    CertDescr loadCert(const QString &data);
    void cleanCerts();

    QVector<CertDescr> certs_;
};

#endif // CERTMANAGER_H
