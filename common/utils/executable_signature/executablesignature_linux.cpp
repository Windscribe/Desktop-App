#include "executablesignature_linux.h"

#include <stdio.h>
#include <stdlib.h>
#include <QCoreApplication>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include "utils/logger.h"

// Buffer for file read operations. The buffer must be able to accomodate
// the RSA signature in whole (e.g. 4096-bit RSA key produces 512 byte signa
#define BUFFER_SIZE 512
static unsigned char buffer[BUFFER_SIZE];

bool ExecutableSignature_linux::isParentProcessGui()
{
    // TODO
    return true;
}

bool ExecutableSignature_linux::verify(const QString &executablePath)
{
    // TODO: bring files into memory at application start up so we don't have to parse/process on the fly
    // TODO: add ExecutableSignature::verify() for openvpn and wireguard
    const QString &filename = executablePath.mid(executablePath.lastIndexOf("/")+1);
    const QString &sigfileQString = QString(QCoreApplication::applicationDirPath() + "/signatures/" + filename + ".sig");
    const QString &pubkeyfileQString = QString(QCoreApplication::applicationDirPath() + "/keys/key.pub");
    qCDebug(LOG_BASIC) << "Verifying " << executablePath << " with " << sigfileQString << " and " << pubkeyfileQString;

    // missing exe, signature or public key
    if (!QFile::exists(executablePath)  ||
        !QFile::exists(sigfileQString)  ||
        !QFile::exists(pubkeyfileQString))
    {
        return false;
    }

    // keep temps around to prevent object deletion
    const std::string fp = executablePath.toStdString();
    const std::string sf = sigfileQString.toStdString();
    const std::string pkf = pubkeyfileQString.toStdString();

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
