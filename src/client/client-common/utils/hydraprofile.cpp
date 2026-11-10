#include "hydraprofile.h"

#include <limits>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"

namespace {

const QString kKind = "windscribe.hydra.profile";
const QString kFileName = "hydra_profile.json";

QString unknownKey(const QJsonObject &obj, const QSet<QString> &allowed)
{
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!allowed.contains(it.key())) {
            return it.key();
        }
    }
    return QString();
}

// Optional string fields may be absent, but not another type.
bool readString(const QJsonObject &obj, const QString &key, bool required, QString &out, QString &error)
{
    if (!obj.contains(key)) {
        if (required) {
            error = QObject::tr("%1 is required").arg(key);
        }
        return !required;
    }
    if (!obj[key].isString()) {
        error = QObject::tr("%1 must be a string").arg(key);
        return false;
    }
    out = obj[key].toString();
    return true;
}

bool readPort(const QJsonObject &obj, const QString &key, uint &out, QString &error)
{
    const QJsonValue v = obj[key];
    const double d = v.toDouble(-1);
    if (!v.isDouble() || d != static_cast<double>(static_cast<qint64>(d)) || d < 1 || d > 65535) {
        error = QObject::tr("%1 must be 1-65535").arg(key);
        return false;
    }
    out = static_cast<uint>(d);
    return true;
}

bool readId(const QJsonObject &obj, const QString &key, int &out, QString &error)
{
    const QJsonValue v = obj[key];
    const double d = v.toDouble(-1);
    if (!v.isDouble() || d != static_cast<double>(static_cast<qint64>(d)) || d < 1 || d > std::numeric_limits<int>::max()) {
        error = QObject::tr("%1 must be a positive whole number").arg(key);
        return false;
    }
    out = static_cast<int>(d);
    return true;
}

bool readHostname(const QJsonObject &obj, const QString &key, bool strict, QString &out, QString &error)
{
    if (!readString(obj, key, true, out, error)) {
        return false;
    }
    if (strict && !HydraProfile::isHostname(out)) {
        error = QObject::tr("%1 must be a hostname").arg(key);
        return false;
    }
    return true;
}

bool readIpv4(const QJsonObject &obj, const QString &key, QString &out, QString &error)
{
    if (!readString(obj, key, true, out, error)) {
        return false;
    }
    if (!NetworkingValidation::isIpv4(out)) {
        error = QObject::tr("%1 must be an IPv4 address").arg(key);
        return false;
    }
    return true;
}

std::optional<HydraProfile::Front> parseFront(const QJsonValue &value, bool strict, QString &error)
{
    if (!value.isObject()) {
        error = QObject::tr("must be an object");
        return std::nullopt;
    }
    const QJsonObject obj = value.toObject();
    // A version 1 front's exit_hostname and exit_ip are unknown here too: a profile names a datacenter,
    // never one of its servers.
    const QString unknown = unknownKey(obj, {"location", "datacenter", "datacenter_id", "exit_port", "entry_ip", "entry_port",
                                             "protocol"});
    if (strict && !unknown.isEmpty()) {
        error = QObject::tr("unknown field %1").arg(unknown);
        return std::nullopt;
    }

    HydraProfile::Front f;
    QString protocol = "wireguard";
    if (!readString(obj, "location", false, f.location, error) ||
        !readHostname(obj, "datacenter", strict, f.datacenter, error) || !readId(obj, "datacenter_id", f.datacenterId, error) ||
        !readPort(obj, "exit_port", f.exitPort, error) ||
        !readIpv4(obj, "entry_ip", f.entryIp, error) || !readPort(obj, "entry_port", f.entryPort, error) ||
        !readString(obj, "protocol", false, protocol, error)) {
        return std::nullopt;
    }
    if (strict && protocol != "wireguard") {
        error = QObject::tr("unsupported protocol %1").arg(protocol);
        return std::nullopt;
    }
    return f;
}

}  // namespace

std::optional<HydraProfile> HydraProfile::parse(const QByteArray &json, QString &error, bool strict)
{
    if (json.size() > kMaxFileSize) {
        error = QObject::tr("The file is too large to be a hydra profile.");
        return std::nullopt;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        error = QObject::tr("Not a hydra profile: the file is not a JSON object.");
        return std::nullopt;
    }
    const QJsonObject obj = doc.object();
    if (strict && obj["kind"].toString() != kKind) {
        error = QObject::tr("Not a hydra profile: kind must be %1.").arg(kKind);
        return std::nullopt;
    }
    if (obj["version"].isDouble() && obj["version"].toDouble() == 1) {
        error = QObject::tr("This hydra profile is from an older hydra and names one server; ask for a new one.");
        return std::nullopt;
    }
    if (strict && (!obj["version"].isDouble() || obj["version"].toDouble() != 2)) {
        error = QObject::tr("Unsupported hydra profile version; this app reads version 2.");
        return std::nullopt;
    }
    const QString unknown = unknownKey(obj, {"version", "kind", "hydra_id", "created_at", "all", "fronts"});
    if (strict && !unknown.isEmpty()) {
        error = QObject::tr("Invalid hydra profile: unknown field %1.").arg(unknown);
        return std::nullopt;
    }

    HydraProfile p;
    QString createdAt;
    QString fieldError;
    if (!readString(obj, "hydra_id", false, p.id_, fieldError) ||
        !readString(obj, "created_at", false, createdAt, fieldError)) {
        error = QObject::tr("Invalid hydra profile: %1.").arg(fieldError);
        return std::nullopt;
    }
    if (obj.contains("all")) {
        if (!obj["all"].isBool()) {
            error = QObject::tr("Invalid hydra profile: all must be true or false.");
            return std::nullopt;
        }
        p.all_ = obj["all"].toBool();
    }
    const QJsonArray fronts = obj["fronts"].toArray();
    if (fronts.isEmpty()) {
        error = QObject::tr("Invalid hydra profile: it has no fronts.");
        return std::nullopt;
    }
    QSet<QString> endpoints;
    for (int i = 0; i < fronts.size(); ++i) {
        const auto front = parseFront(fronts[i], strict, fieldError);
        if (!front) {
            error = QObject::tr("Invalid hydra profile: front %1: %2.").arg(i + 1).arg(fieldError);
            return std::nullopt;
        }
        if (strict && endpoints.contains(front->endpoint())) {
            error = QObject::tr("Invalid hydra profile: front %1: entry %2 is listed twice.").arg(i + 1).arg(front->endpoint());
            return std::nullopt;
        }
        endpoints.insert(front->endpoint());
        p.fronts_ << *front;
    }
    p.json_ = json;
    return p;
}

QString HydraProfile::redactIp(const QString &ip)
{
    const int dot = ip.lastIndexOf('.');
    return dot < 0 ? QString("***") : ip.left(dot) + ".***";
}

bool HydraProfile::isHostname(const QString &name)
{
    if (name.size() > 253 || !name.contains('.')) {
        return false;
    }
    for (const QString &label : name.split('.')) {
        if (label.isEmpty() || label.size() > 63 || label.startsWith('-') || label.endsWith('-')) {
            return false;
        }
        for (const QChar c : label) {
            if (c.unicode() > 127 || !(c.isLetterOrNumber() || c == '-')) {
                return false;
            }
        }
    }
    return true;
}

const HydraProfile::Front *HydraProfile::frontForDatacenter(int datacenterId) const
{
    if (datacenterId <= 0) {
        return nullptr;
    }
    for (const Front &f : fronts_) {
        if (f.datacenterId == datacenterId) {
            return &f;
        }
    }
    return nullptr;
}

QStringList HydraProfile::summary(const std::function<QString(int datacenterId)> &serverListName) const
{
    QStringList lines;
    for (const Front &f : fronts_) {
        const QString name = serverListName ? serverListName(f.datacenterId) : QString();
        if (!name.isEmpty()) {
            lines << QObject::tr("%1 via %2").arg(name, f.endpoint());
        } else {
            // No connection goes through this front, so only here is the profile's free-text label shown.
            lines << QObject::tr("%1 via %2 (not in the server list)").arg(f.location.isEmpty() ? f.datacenter : f.location,
                                                                           f.endpoint());
        }
    }
    return lines;
}

QString HydraProfile::path()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + kFileName;
}

std::optional<HydraProfile> HydraProfile::load(QString *error)
{
    QFile file(path());
    if (!file.exists()) {
        return std::nullopt;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(LOG_BASIC) << "Could not read the hydra profile:" << file.errorString();
        if (error) {
            *error = QObject::tr("Could not read the hydra profile: %1").arg(file.errorString());
        }
        return std::nullopt;
    }
    QString parseError;
    auto p = parse(file.read(kMaxFileSize + 1), parseError, false);
    if (!p) {
        qCWarning(LOG_BASIC) << "Can't read the stored hydra profile:" << parseError;
        if (error) {
            *error = parseError;
        }
    }
    return p;
}

std::optional<HydraProfile> HydraProfile::save(const QByteArray &json, QString &error)
{
    auto p = parse(json, error);
    if (!p) {
        return std::nullopt;
    }
    QDir().mkpath(QFileInfo(path()).absolutePath());
    QSaveFile file(path());
    if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size() || !file.commit()) {
        error = QObject::tr("Could not save the hydra profile: %1").arg(file.errorString());
        return std::nullopt;
    }
    return p;
}

bool HydraProfile::remove()
{
    return !QFile::exists(path()) || QFile::remove(path());
}

HydraProfile::Locations HydraProfile::locations(const std::function<QString(int datacenterId)> &serverListName) const
{
    Locations out;
    if (all_) {
        out.all = true;
        return out;
    }
    for (const Front &f : fronts_) {
        if (out.shown.size() == kShownLocations) {
            ++out.more;
            continue;
        }
        const QString name = serverListName ? serverListName(f.datacenterId) : QString();
        out.shown << (name.isEmpty() ? QObject::tr("%1 (not in the server list)").arg(f.location.isEmpty() ? f.datacenter : f.location)
                                     : name);
    }
    return out;
}
