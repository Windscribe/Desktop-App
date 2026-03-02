#pragma once

#include <QString>

namespace LoginWindow {

class HashUtil
{
public:
    // Calculates the truncated SHA256 hash of a file
    // Returns "0x" followed by the second half of the SHA256 hash (32 hex characters)
    // Returns empty string if file cannot be opened
    static QString getTruncatedSHA256(const QString &filePath);

    // Validates that a string is a valid truncated SHA256 hash
    // Format: "0x" followed by exactly 32 hexadecimal characters (lowercase)
    static bool isValidTruncatedSHA256(const QString &hash);

    // Generates a random truncated hash in the correct format
    // Returns "0x" followed by 32 random hexadecimal characters (lowercase)
    static QString generateRandomTruncatedHash();
};

} // namespace LoginWindow
