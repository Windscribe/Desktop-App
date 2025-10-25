#include "controlddevices.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace api_responses {

ControldDevices::ControldDevices(const std::string &json, int httpResponseCode)
{
    // Check HTTP response code first
    if (httpResponseCode != 200) {
        result_ = CONTROLD_FETCH_AUTH_ERROR;
        return;
    }

    // Parse JSON response
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json), &errCode);

    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        result_ = CONTROLD_FETCH_AUTH_ERROR;
        return;
    }

    QJsonObject root = doc.object();

    // Check for error in response or missing body
    if (root.contains("error") || !root.contains("body")) {
        result_ = CONTROLD_FETCH_AUTH_ERROR;
        return;
    }

    if (root.contains("body") && root["body"].isObject()) {
        QJsonObject body = root["body"].toObject();
        if (body.contains("devices") && body["devices"].isArray()) {
            QJsonArray devicesArray = body["devices"].toArray();

            for (const QJsonValue &deviceVal : devicesArray) {
                if (deviceVal.isObject()) {
                    QJsonObject device = deviceVal.toObject();
                    if (device.contains("name") && device.contains("resolvers") &&
                        device["resolvers"].isObject()) {
                        QString name = device["name"].toString();
                        QJsonObject resolvers = device["resolvers"].toObject();
                        if (resolvers.contains("doh") && resolvers["doh"].isString()) {
                            QString doh = resolvers["doh"].toString();
                            devices_.append(qMakePair(name, doh));
                        }
                    }
                }
            }
        }
    }

    result_ = CONTROLD_FETCH_SUCCESS;
}

} // namespace api_responses
