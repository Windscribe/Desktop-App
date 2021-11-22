#include "executablesignature_linux.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#ifdef QT_CORE_LIB
    #include <QCoreApplication>
    #include "utils/logger.h"
#endif

// Buffer for file read operations. The buffer must be able to accomodate
// the RSA signature in whole (e.g. 4096-bit RSA key produces 512 byte signa
#define BUFFER_SIZE 512
static unsigned char buffer[BUFFER_SIZE];

class EvpBioCharBuf
{
public:
   EvpBioCharBuf(void);
   ~EvpBioCharBuf(void);

   bool isValid(void) const { return pBIO_ != NULL; }
   BIO* getBIO() const { return pBIO_; }
   int write(const void *data, int dlen);

private:
   BIO* pBIO_;
};

EvpBioCharBuf::EvpBioCharBuf(void)
{
   pBIO_ = BIO_new(BIO_s_mem());
}

EvpBioCharBuf::~EvpBioCharBuf(void)
{
   if (pBIO_ != NULL) {
      BIO_free_all(pBIO_);
   }
}

int EvpBioCharBuf::write(const void *data, int dlen)
{
    if (pBIO_ != NULL) {
        return BIO_write(pBIO_, data, dlen);
    }

    return 0;
}

bool ExecutableSignature_linux::verifyWithPublicKey(const std::string &exePath, const std::string &sigPath, const std::string &pubKeyBytes)
{
    // key.pub is 800 bytes on disk
    if (pubKeyBytes.length() > 800)
    {
        lastError_ = "Invalid public key, size is too large: " + std::to_string(pubKeyBytes.length());
        return false;
    }

    // read public key into openssl bio abstraction
    EvpBioCharBuf bioPublicKey;
    if (!bioPublicKey.isValid())
    {
        lastError_ = "Failed to allocate an OpenSSL BIO buffer";
        return false;
    }

    if (bioPublicKey.write(pubKeyBytes.data(), pubKeyBytes.length()) <= 0)
    {
        lastError_ = "Failed to write public key resource to bio";
        return false;
    }

    // keep temps around to prevent object deletion
    const char* filenamePath = exePath.c_str();
    const char* sigfile = sigPath.c_str();

    // Calculate SHA256 digest for datafile
    FILE* datafile = fopen(filenamePath , "rb");
    if (datafile == NULL)
    {
        lastError_ = "Failed to open executable for reading: " + std::to_string(errno);
        return false;
    }

    // Buffer to hold the calculated digest
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    // Read binary data in chunks and feed it to OpenSSL SHA256
    unsigned bytes = 0;
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, datafile)))
    {
        SHA256_Update(&ctx, buffer, bytes);
    }
    SHA256_Final(digest, &ctx);
    fclose(datafile);

    // Read signature from file
    FILE* sign = fopen(sigfile , "r");
    if (sign == NULL)
    {
        lastError_ = "Failed to open signature file for reading: " + std::to_string(errno);
        return false;
    }

    bytes = fread(buffer, 1, BUFFER_SIZE, sign);
    fclose(sign);

    if (bytes != BUFFER_SIZE)
    {
        std::ostringstream stream;
        stream << "Signature file is an invalid size, or failed to read entire file. Expected " << BUFFER_SIZE << "bytes, read " << bytes << ".";
        lastError_ = stream.str();
        return false;
    }

    // Verify that calculated digest and signature match
    RSA* rsa_pubkey = PEM_read_bio_RSA_PUBKEY(bioPublicKey.getBIO(), NULL, NULL, NULL);
    if (rsa_pubkey == NULL)
    {
        lastError_ = "Failed to read the RSA public key";
        return false;
    }

    // Decrypt signature (in buffer) and verify it matches
    // with the digest calculated from data file.
    int result = RSA_verify(NID_sha256, digest, SHA256_DIGEST_LENGTH, buffer, bytes, rsa_pubkey);
    RSA_free(rsa_pubkey);

    if (result != 1) {
        lastError_ = "Executable's signature does not match signature file";
    }

    return (result == 1);
}

#ifdef QT_CORE_LIB

bool ExecutableSignature_linux::isParentProcessGui()
{
    // TODO
    return true;
}

// Assumed hierarchy:
/* INSTALL_LOCATION = /usr/local/windscribe (default)
 *      signatures/
 *          WindscribeEngine.sig
 *      WindscribeEngine
 */
bool ExecutableSignature_linux::verify(const QString &executablePath)
{
    // TODO: bring files into memory at application start up so we don't have to parse/process on the fly

#if defined(USE_SIGNATURE_CHECK)
    const QString &keyPath = ":/keys/linux/key.pub";
    const QString &filename = executablePath.mid(executablePath.lastIndexOf("/")+1);
    const QString &sigfile = QCoreApplication::applicationDirPath() + "/signatures/" + filename + ".sig";
    return verifyWithPublicKeyFromResources(executablePath, sigfile, keyPath);
#else
    Q_UNUSED(executablePath)
    // testing only
    // This debug block should never be called, but keeping around for testing
    // assumes we have an installed version of windscribe with matching signature in /usr/local/windscribe
//    const QString testName = "WindscribeEngine";
//    const QString sigfile = QString("/usr/local/windscribe/signatures/%1.sig").arg(testName);
//    const QString executablePath2 = "/usr/local/windscribe/" + testName;
//    const QString &keyPath = "/home/parallels/work/client-desktop/common/keys/linux/key.pub";
//    return verifyWithPublicKeyFromFilesystem(executablePath2, sigfile, keyPath);
    return true; // if we somehow call in debug mode
#endif

}

bool ExecutableSignature_linux::verifyWithPublicKeyFromResources(const QString &executablePath, const QString &signaturePath, const QString &publicKeyPath)
{
    qCDebug(LOG_BASIC) << "Verifying " << executablePath << " with " << signaturePath << " and " << publicKeyPath;

    // check for missing exe, signature or public key
    if (!QFile::exists(executablePath) ||
        !QFile::exists(signaturePath)  ||
        !QFile::exists(publicKeyPath)  )
    {
        qCDebug(LOG_BASIC) << "Verification failed: binary, signature or public key does not exist";
        return false;
    }

    // open and read in public key
    QFile publicKeyResource(publicKeyPath);
    if (!publicKeyResource.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCDebug(LOG_BASIC) << "Failed to open public key resource for reading";
        return false;
    }
    const QByteArray &publicKeyBytes = publicKeyResource.readAll(); // cannot access file descriptor/handle of a resource so just copy the bytes
    publicKeyResource.close();

    ExecutableSignature_linux verifySignature;
    bool result = verifySignature.verifyWithPublicKey(executablePath.toStdString(), signaturePath.toStdString(), publicKeyBytes.toStdString());
    if (!result) {
        qCDebug(LOG_BASIC) << verifySignature.lastError().c_str();
    }
    return result;
}

bool ExecutableSignature_linux::verifyWithPublicKeyFromFilesystem(const QString &executablePath, const QString &signaturePath, const QString &publicKeyPath)
{
    qCDebug(LOG_BASIC) << "Verifying " << executablePath << " with " << signaturePath << " and " << publicKeyPath;

    // missing exe, signature or public key
    if (!QFile::exists(executablePath)  ||
        !QFile::exists(signaturePath)  ||
        !QFile::exists(publicKeyPath))
    {
        qCDebug(LOG_BASIC) << "Verification failed: binary, signature or public key does not exist";
        return false;
    }

    // open public key for reading
    std::ifstream pubkeyfile;
    pubkeyfile.open(publicKeyPath.toStdString(), std::ifstream::in);
    if (!pubkeyfile.is_open())
    {
        qCDebug(LOG_BASIC) << "Failed to open public key for reading";
        return false;
    }

    // read public key into string
    std::string pubkeyData((std::istreambuf_iterator<char>(pubkeyfile) ),
                           (std::istreambuf_iterator<char>()    ) );
    pubkeyfile.close();

    ExecutableSignature_linux verifySignature;
    bool result = verifySignature.verifyWithPublicKey(executablePath.toStdString(), signaturePath.toStdString(), pubkeyData);
    if (!result) {
        qCDebug(LOG_BASIC) << verifySignature.lastError().c_str();
    }
    return result;
}

#endif // QT_CORE_LIB
