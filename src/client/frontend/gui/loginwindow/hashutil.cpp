#include "hashutil.h"

#include <QFile>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QRegularExpression>
#include "utils/log/categories.h"

namespace LoginWindow {

QString HashUtil::getTruncatedSHA256(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(LOG_BASIC) << "Could not open file:" << filePath;
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    // addData() fails on a mid-read I/O error; returning a hash of partial content would create an unreproducible credential.
    if (!hash.addData(&file)) {
        qCWarning(LOG_BASIC) << "Could not read file:" << filePath;
        return QString();
    }

    // SHA256 hex = 64 characters; take the second half (the last 32 characters out of 64)
    return "0x" + QString::fromLatin1(hash.result().toHex().last(32));
}

bool HashUtil::isValidTruncatedSHA256(const QString &hash)
{
    static const QRegularExpression kTruncatedHashRegex("\\A0x[0-9a-f]{32}\\z");
    return kTruncatedHashRegex.match(hash).hasMatch();
}

QString HashUtil::generateRandomTruncatedHash()
{
    quint32 buf[4];
    QRandomGenerator::system()->fillRange(buf);
    return "0x" + QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(buf), sizeof(buf)).toHex());
}

} // namespace LoginWindow
