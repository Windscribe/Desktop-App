#include "checkupdate.h"
#include <QMetaType>

const int typeIdCheckUpdate = qRegisterMetaType<types::CheckUpdate>("types::CheckUpdate");

namespace types {

bool CheckUpdate::initFromJson(QJsonObject &json, QString &outErrorMessage)
{
    // check for required fields in json
    QStringList required_fields({"current_version", "supported", "latest_version", "latest_build", "is_beta",
                                 "min_version", "update_needed_flag", "update_url"});
    for (const QString &str : required_fields)
    {
        if (!json.contains(str))
        {
            outErrorMessage = str + " field not found";
            return false;
        }
    }

    d->cui_.set_is_available(true); // is_available used as success indicator in gui
    d->cui_.set_version(json["latest_version"].toString().toStdString());
    d->cui_.set_latest_build(json["latest_build"].toInt());
    d->cui_.set_update_channel(static_cast<ProtoTypes::UpdateChannel>(json["is_beta"].toInt()));
    d->cui_.set_url(json["update_url"].toString().toStdString());
    d->cui_.set_is_supported(json["supported"].toInt() == 1);

    if (json.contains("sha256"))
    {
        d->cui_.set_sha256(json["sha256"].toString().toStdString());
    }
    d->isInitialized_ = true;
    return true;
}

void CheckUpdate::initFromProtoBuf(const ProtoTypes::CheckUpdateInfo &cui)
{
    d->cui_ = cui;
    d->isInitialized_ = true;
}

ProtoTypes::CheckUpdateInfo CheckUpdate::getProtoBuf() const
{
    Q_ASSERT(d->isInitialized_);
    return d->cui_;
}

QString CheckUpdate::getUrl() const
{
    Q_ASSERT(d->isInitialized_);
    return QString::fromStdString(d->cui_.url());
}

QString CheckUpdate::getSha256() const
{
    Q_ASSERT(d->isInitialized_);
    return QString::fromStdString(d->cui_.sha256());
}

bool CheckUpdate::isInitialized() const
{
    return d->isInitialized_;
}


} // apiinfo namespace
