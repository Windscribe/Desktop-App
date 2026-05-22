#pragma once

#include <QString>

namespace customconfigs {

class OvpnCredentialInliner
{
public:
    struct Result
    {
        bool success;
        QString config;        // valid when success is true
        QString errorMessage;  // valid when success is false
    };

    static Result process(const QString &config, const QString &resolutionBaseDir);
};

} // namespace customconfigs
