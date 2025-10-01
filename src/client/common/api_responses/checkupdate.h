#pragma once

#include <QString>
#include <QJsonObject>
#include "types/enums.h"

namespace api_responses {

class CheckUpdate
{
public:
    CheckUpdate(const std::string &json);

    bool operator==(const CheckUpdate &other) const;
    bool operator!=(const CheckUpdate &other) const;

    QString version() const { return version_; }
    UPDATE_CHANNEL updateChannel() const { return updateChannel_; }
    int latestBuild() const { return latestBuild_; }
    QString url() const { return url_; }
    QString sha256() const { return sha256_; }
    bool isSupported() const { return isSupported_; }
    bool isAvailable() const { return isAvailable_; }

private:
    bool isAvailable_ = false;
    QString version_;
    UPDATE_CHANNEL updateChannel_ = UPDATE_CHANNEL_RELEASE;
    int latestBuild_ = 0;
    QString url_;
    bool isSupported_ = false;
    QString sha256_;
};

} // api_responses namespace
