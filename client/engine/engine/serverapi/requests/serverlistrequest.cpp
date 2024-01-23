#include "serverlistrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>

#include "engine/utils/urlquery_utils.h"
#include "utils/extraconfig.h"
#include "utils/logger.h"

namespace server_api {

ServerListRequest::ServerListRequest(QObject *parent, const QString &language, const QString &revision, bool isPro,
                                     const QStringList &alcList,  IConnectStateController *connectStateController) :
    BaseRequest(parent, RequestType::kGet),
    language_(language),
    revision_(revision),
    isPro_(isPro),
    alcList_(alcList),
    connectStateController_(connectStateController)
{
    isFromDisconnectedVPNState_ = (connectStateController_->currentState() == CONNECT_STATE::CONNECT_STATE_DISCONNECTED);
}

QUrl ServerListRequest::url(const QString &domain) const
{
    // generate alc parameter, if not empty
    QString alcField;
    if (!alcList_.isEmpty()) {
        for (int i = 0; i < alcList_.count(); ++i) {
            alcField += alcList_[i];
            if (i < (alcList_.count() - 1)) {
                alcField += ",";
            }
        }
    }

    QUrlQuery query;
    if (!alcField.isEmpty()) {
        query.addQueryItem("alc", alcField);
    }

    QString countryOverride;
    if (ExtraConfig::instance().serverListIgnoreCountryOverride()) {
        // Instruct the serverlist endpoint to ignore geolocation based on our IP.
        countryOverride = "ZZ";
    }
    else {
        auto extraConfigCountryOverride = ExtraConfig::instance().serverlistCountryOverride();
        if (extraConfigCountryOverride.has_value()) {
            countryOverride = extraConfigCountryOverride.value();
        }
        else if (connectStateController_->currentState() != CONNECT_STATE::CONNECT_STATE_DISCONNECTED) {
            QSettings settings;
            if (settings.contains("countryOverride")) {
                countryOverride = settings.value("countryOverride").toString();
            }
        }
    }

    if (!countryOverride.isEmpty()) {
        query.addQueryItem("country_override", countryOverride);
        qCDebug(LOG_SERVER_API) << "API request ServerLocations added countryOverride =" << countryOverride;
    }

    QString strIsPro = isPro_ ? "1" : "0";
    QUrl url = QUrl("https://" + hostname(domain, SudomainType::kAssets) + "/serverlist/mob-v2/" + strIsPro + "/" + revision_);
    url.setQuery(query);

    return url;
}

QString ServerListRequest::name() const
{
    return "ServerList";
}

void ServerListRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonObject = doc.object();

    if (!jsonObject.contains("info")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json (info field not found)";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json (data field not found)";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    // parse revision number
    QJsonObject jsonInfo = jsonObject["info"].toObject();
    bool isChanged = jsonInfo["changed"].toInt() != 0;
    int newRevision = jsonInfo["revision"].toInt();
    QString revisionHash = jsonInfo["revision_hash"].toString();

    // manage the country override flag according to the documentation
    // https://gitlab.int.windscribe.com/ws/client/desktop/client-desktop-public/-/issues/354
    if (jsonInfo.contains("country_override")) {
        if (isFromDisconnectedVPNState_ && connectStateController_->currentState() == CONNECT_STATE::CONNECT_STATE_DISCONNECTED) {
            QSettings settings;
            settings.setValue("countryOverride", jsonInfo["country_override"].toString());
            qCDebug(LOG_SERVER_API) << "API request ServerLocations saved countryOverride =" << jsonInfo["country_override"].toString();
        }
    } else {
        if (isFromDisconnectedVPNState_ && connectStateController_->currentState() == CONNECT_STATE::CONNECT_STATE_DISCONNECTED) {
            QSettings settings;
            if (settings.contains("countryOverride")) {
                settings.remove("countryOverride");
                qCDebug(LOG_SERVER_API) << "API request ServerLocations removed countryOverride flag";
            }
        }
    }

    if (isChanged)  {
        qCDebug(LOG_SERVER_API) << "API request ServerLocations successfully executed, revision changed =" << newRevision
                                << ", revision_hash =" << revisionHash;

        // parse locations array
        const QJsonArray jsonData = jsonObject["data"].toArray();

        for (int i = 0; i < jsonData.size(); ++i) {
            if (jsonData.at(i).isObject()) {
                apiinfo::Location sl;
                QJsonObject dataElement = jsonData.at(i).toObject();
                if (sl.initFromJson(dataElement, forceDisconnectNodes_)) {
                    locations_ << sl;
                } else {
                    QJsonDocument invalidData(dataElement);
                    qCDebug(LOG_SERVER_API) << "API request ServerLocations skipping invalid/incomplete 'data' element at index" << i
                                            << "(" << invalidData.toJson(QJsonDocument::Compact) << ")";
                }
            } else {
                qCDebug(LOG_SERVER_API) << "API request ServerLocations skipping non-object 'data' element at index" << i;
            }
        }

        if (locations_.empty())  {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json, no valid 'data' elements were found";
            setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        }
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "API request ServerLocations successfully executed, revision not changed";
    }
}

QVector<apiinfo::Location> ServerListRequest::locations() const
{
    return locations_;
}

QStringList ServerListRequest::forceDisconnectNodes() const
{
    return forceDisconnectNodes_;
}

} // namespace server_api {
