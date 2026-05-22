#pragma once

#include <QByteArray>
#include <QString>
#include <algorithm>

namespace ProxyAuth {

struct Config
{
    bool required = false;
    QString username;
    QString password;
};

// Constant-time byte compare. Loops over max(a.size(), b.size()) without an early exit on size mismatch, so timing
// doesn't reveal the stored length any more precisely than the larger of the two inputs.
inline bool secureEqual(const QByteArray &a, const QByteArray &b)
{
    const int n = (std::max)(a.size(), b.size());
    unsigned int diff = static_cast<unsigned int>(a.size() ^ b.size());
    for (int i = 0; i < n; ++i) {
        const unsigned char ax = (i < a.size()) ? static_cast<unsigned char>(a[i]) : 0;
        const unsigned char bx = (i < b.size()) ? static_cast<unsigned char>(b[i]) : 0;
        diff |= static_cast<unsigned int>(ax ^ bx);
    }
    return diff == 0;
}

}  // namespace ProxyAuth
