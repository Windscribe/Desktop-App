#include "certmanager.h"
#include <QFile>
#include "utils/logger.h"

CertManager::CertManager()
{
    // load certificates from bundle
    {
        QFile file(":/resources/cert/certs_bundle.pem");
        if (file.open(QIODeviceBase::ReadOnly))
        {
            QByteArray arr = file.readAll();
            parseCertsBundle(arr);
            file.close();
        }
        else
        {
            qCDebug(LOG_BASIC) << "can't load SSL certificates from resources";
        }
    }

    // load Windscribe certificates
    {
        QFile file(":/resources/cert/cert.crt");
        if (file.open(QIODeviceBase::ReadOnly))
        {
            QByteArray arr = file.readAll();
            parseCertsBundle(arr);
            file.close();
        }
        else
        {
            qCDebug(LOG_BASIC) << "can't load SSL certificates from resources";
        }
    }
}

CertManager::~CertManager()
{
    cleanCerts();
}

int CertManager::count()
{
    return certs_.count();
}

X509 *CertManager::getCert(int ind)
{
    return certs_[ind].cert;
}

void CertManager::parseCertsBundle(QByteArray &arr)
{
    QString s = arr;
    int curOffs = 0;

    while (true)
    {
        int indStart = s.indexOf("-----BEGIN CERTIFICATE-----", curOffs);
        int indEnd = s.indexOf("-----END CERTIFICATE-----", indStart);

        if (indStart != -1 && indEnd != -1)
        {
            QString cert = s.mid(indStart, indEnd - indStart + QString("-----END CERTIFICATE-----").length());
            certs_ << loadCert(cert);
            curOffs = indEnd + QString("-----END CERTIFICATE-----").length();
        }
        else
        {
            break;
        }
    }
}

CertManager::CertDescr CertManager::loadCert(const QString &data)
{
    CertDescr cd;
    cd.cert = NULL;
    std::string sss = data.toStdString();
    const char *mypem = sss.c_str();

    cd.bio = BIO_new_mem_buf(mypem, -1);

    PEM_read_bio_X509(cd.bio, &cd.cert, 0, NULL);
    if(cd.cert == NULL)
    {
        qCDebug(LOG_BASIC) << "PEM_read_bio_X509 failed...";
    }

    return cd;
}

void CertManager::cleanCerts()
{
    for (int i = 0; i < certs_.count(); ++i)
    {
        X509_free(certs_[i].cert);
        BIO_free(certs_[i].bio);
    }
    certs_.clear();
}
