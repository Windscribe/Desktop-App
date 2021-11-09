#include "executablesignature_linux.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
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

bool ExecutableSignature_linux::verifyWithPublicKey(const std::string &exePath, const std::string &sigPath, const std::string &pubKeyBytes)
{
    // read public key into openssl bio abstraction
    BIO *bioPublicKey = BIO_new(BIO_s_mem());
    if (BIO_write(bioPublicKey, pubKeyBytes.data(), pubKeyBytes.length()) <= 0)
    {
        std::cout << "Failed to write public key resource to bio";
        BIO_free(bioPublicKey);
        return false;
    }

    // keep temps around to prevent object deletion
    const char* filenamePath = exePath.c_str();
    const char* sigfile = sigPath.c_str();

    // Calculate SHA256 digest for datafile
    FILE* datafile = fopen(filenamePath , "rb");
    if (datafile == NULL)
    {
        std::cout << "Failed to open executable for reading";
        BIO_free(bioPublicKey);
        return false;
    }

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
    FILE* sign = fopen(sigfile , "r");
    if (sign == NULL)
    {
        std::cout << "Failed to open signature for reading";
        BIO_free(bioPublicKey);
        return false;
    }
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

#ifdef QT_DEBUG
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
#else
    const QString &keyPath = ":/keys/linux/key.pub";
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

    return verifyWithPublicKey(executablePath.toStdString(), signaturePath.toStdString(), publicKeyBytes.toStdString());
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

    return verifyWithPublicKey(executablePath.toStdString(), signaturePath.toStdString(), pubkeyData);
}

#endif // QT_CORE_LIB
