#include "hashutil.h"

#include <QFile>
#include <QCryptographicHash>
#include "utils/log/categories.h"
#include "utils/utils.h"

namespace LoginWindow {

QString HashUtil::getTruncatedSHA256(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(LOG_BASIC) << "Could not open file:" << filePath;
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);

    while (!file.atEnd()) {
        hash.addData(file.read(8192)); // read 8KB at a time
    }

    file.close();

    // Receive the full hash as a string
    QByteArray fullHash = hash.result();
    QString fullHashString = fullHash.toHex(); // SHA256 = 64 characters (32 bytes * 2)

    // Take the second half (the last 32 characters out of 64)
    int halfLength = fullHashString.length() / 2;
    QString secondHalf = fullHashString.mid(halfLength);

    return "0x" + secondHalf;
}

bool HashUtil::isValidTruncatedSHA256(const QString &hash)
{
    if (hash.length() != 34) {
        return false;
    }

    if (!hash.startsWith("0x")) {
        return false;
    }

    for (int i = 2; i < hash.length(); ++i) {
        QChar c = hash[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }

    return true;
}

QString HashUtil::generateRandomTruncatedHash()
{
    static const char hexChars[] = "0123456789abcdef";
    QString hash = "0x";
    
    // Generate 32 random hexadecimal characters
    for (int i = 0; i < 32; ++i) {
        int index = Utils::generateIntegerRandom(0, 15); // 0-15
        hash += hexChars[index];
    }
    
    return hash;
}

} // namespace LoginWindow
