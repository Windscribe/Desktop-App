#include "executablesignature_linux.h"

#include <stdio.h>
#include <stdlib.h>
#include <QCoreApplication>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include "utils/logger.h"

// Buffer for file read operations. The buffer must be able to accomodate
// the RSA signature in whole (e.g. 4096-bit RSA key produces 512 byte signa
#define BUFFER_SIZE 512
static unsigned char buffer[BUFFER_SIZE];

// Assumed hierarchy:
/* INSTALL_LOCATION = /usr/local/windscribe (default)
 *      signatures/
 *          WindscribeEngine.sig
 *      WindscribeEngine
 */

bool ExecutableSignature_linux::isParentProcessGui()
{
    // TODO
    return true;
}

bool ExecutableSignature_linux::verify(const QString &executablePath)
{
    // TODO: bring files into memory at application start up so we don't have to parse/process on the fly

    const QString &keyPath = ":/keys/linux/key.pub";

#ifdef QT_DEBUG
    Q_UNUSED(executablePath)
    Q_UNUSED(keyPath)
    // testing only
    // This debug block should never be called, but keeping around for testing
    // assumes we have an installed version of windscribe with matching signature in /usr/local/windscribe
//    const QString &sigfile = "/usr/local/windscribe/signatures/windscribestunnel.sig";
//    const QString &executablePath2 = "/usr/local/windscribe/windscribestunnel";
//    return verifyWithPublicKeyFromResources(executablePath2, sigfile, keyPath);
    return true; // if we somehow call in debug mode
#else
    const QString &filename = executablePath.mid(executablePath.lastIndexOf("/")+1);
    const QString &sigfile = QCoreApplication::applicationDirPath() + "/signatures/" + filename + ".sig";
    return verifyWithPublicKeyFromResources(executablePath, sigfile, keyPath);
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

    // read public key into openssl bio abstraction
    BIO *bioPublicKey = BIO_new(BIO_s_mem());
    if (BIO_write(bioPublicKey, publicKeyBytes.data(), publicKeyBytes.length()) <= 0)
    {
        qCDebug(LOG_BASIC) << "Failed to write public key resource to bio";
        return false;
    }

    // keep temps around to prevent object deletion
    const std::string fp = executablePath.toStdString();
    const std::string sf = signaturePath.toStdString();
    const char* filenamePath = fp.c_str();
    const char* sigfile = sf.c_str();

    // Calculate SHA256 digest for datafile
    FILE* datafile = fopen(filenamePath , "rb");

    // Buffer to hold the calculated digest
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    // Read binary data in chunks and feed it to OpenSSL SHA256
    unsigned bytes = 0;
    while((bytes = fread(buffer, 1, BUFFER_SIZE, datafile)))
    {
        SHA256_Update(&ctx, buffer, bytes);
    }
    SHA256_Final(digest, &ctx);
    fclose(datafile);

    // Read signature from file
    FILE* sign = fopen (sigfile , "r");
    bytes = fread(buffer, 1, BUFFER_SIZE, sign);
    fclose(sign);

    // Verify that calculated digest and signature match    
    RSA* rsa_pubkey = PEM_read_bio_RSA_PUBKEY(bioPublicKey, NULL, NULL, NULL);

    // Decrypt signature (in buffer) and verify it matches
    // with the digest calculated from data file.
    int result = RSA_verify(NID_sha256, digest, SHA256_DIGEST_LENGTH,
                            buffer, bytes, rsa_pubkey);
    RSA_free(rsa_pubkey);
    BIO_free(bioPublicKey);

    // signature invalid
    if(result != 1)
    {
        return false;
    }
    return true;
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

    // keep temps around to prevent object deletion
    const std::string fp = executablePath.toStdString();
    const std::string sf = signaturePath.toStdString();
    const std::string pkf = publicKeyPath.toStdString();

    const char* filenamePath = fp.c_str();
    const char* sigfile = sf.c_str();
    const char* pubkeyfile = pkf.c_str();

    unsigned bytes = 0;

    // Calculate SHA256 digest for datafile
    FILE* datafile = fopen(filenamePath , "rb");

    // Buffer to hold the calculated digest
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    // Read binary data in chunks and feed it to OpenSSL SHA256
    while((bytes = fread(buffer, 1, BUFFER_SIZE, datafile)))
    {
        SHA256_Update(&ctx, buffer, bytes);
    }
    SHA256_Final(digest, &ctx);
    fclose(datafile);

    // Read signature from file
    FILE* sign = fopen (sigfile , "r");
    bytes = fread(buffer, 1, BUFFER_SIZE, sign);
    fclose(sign);

    // Verify that calculated digest and signature match
    // Read public key from file
    FILE* pubkey = fopen(pubkeyfile, "r");
    RSA* rsa_pubkey = PEM_read_RSA_PUBKEY(pubkey, NULL, NULL, NULL);

    // rsa_pubkey == NULL on failure, failure will occur when feeding 404 html into this function
    if (!rsa_pubkey)
    {
        qCDebug(LOG_BASIC) << "Failed to read in public key";
        return false;
    }

    // Decrypt signature (in buffer) and verify it matches
    // with the digest calculated from data file.
    int result = RSA_verify(NID_sha256, digest, SHA256_DIGEST_LENGTH,
                            buffer, bytes, rsa_pubkey);
    RSA_free(rsa_pubkey);
    fclose(pubkey);

    // signature invalid
    if(result != 1)
    {
        return false;
    }
    return true;
}
