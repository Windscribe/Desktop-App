#pragma once

#include <string>
#include <QList>
#include <QPair>
#include <QString>
#include "types/enums.h"

namespace api_responses {

class ControldDevices
{
public:
    ControldDevices(const std::string &json, int httpResponseCode);

    CONTROLD_FETCH_RESULT result() const { return result_; }
    QList<QPair<QString, QString>> devices() const { return devices_; }

private:
    CONTROLD_FETCH_RESULT result_;
    QList<QPair<QString, QString>> devices_;  // List of (name, doh_resolver) pairs
};

} //namespace api_responses
