#include "wireguardcustomconfig.h"
#include "utils/log/categories.h"
#include "utils/log/clean_sensitive_info.h"
#include "utils/networkingvalidation.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>

namespace customconfigs {

// Maximum file size for a WireGuard .conf (no legitimate config approaches this).
static constexpr qint64 kMaxConfigFileSize = 64 * 1024;

// WireGuard keys are 32 bytes, base64-encoded = 44 characters (including trailing '=').
// anchoredPattern uses \A...\z so a trailing newline (which $ would tolerate) is rejected.
static const QRegularExpression kBase64KeyRegex(QRegularExpression::anchoredPattern(QStringLiteral("[A-Za-z0-9+/]{43}=")));

// H1-H4 are either a single uint32 decimal string or a range "x-y" of two uint32 decimal strings.
// See: https://github.com/amnezia-vpn/amneziawg-go (message paddings).
static const QRegularExpression kHeaderValueRegex(QRegularExpression::anchoredPattern(QStringLiteral("[0-9]{1,10}(-[0-9]{1,10})?")));

// I1-I5 are tag-syntax init-packet specs: <b 0xHEX>, <r N>, <rd N>, <rc N>, <t>
// (see amneziawg-go README). MTU bounds the expanded payload to ~1500 bytes;
// 4096 leaves comfortable margin for the literal string form.
static constexpr int kMaxIValueLength = 4096;

// AmneziaWG 3.x range parameters: "N" or "N-M" of uint32 with N <= M (amneziawg-go UintRange).
static const QRegularExpression kUintRangeRegex(QRegularExpression::anchoredPattern(QStringLiteral("([0-9]{1,10})(?:-([0-9]{1,10}))?")));

// Vanilla configs run on the WireGuard kernel module or wireguard-nt, which take a uint16; amneziawg-go takes a uint32.
static constexpr quint32 kMaxVanillaPersistentKeepalive = 65535;

// With header protection on, amneziawg-go takes the cipher nonce from the S1-S4 padding and rejects the whole
// config if any of them is shorter than the nonce (HeaderCipherNonceSize).
static constexpr int kMinHeaderProtectionPadding = 12;

static bool isValidWgKey(const QString &key)
{
    return kBase64KeyRegex.match(key).hasMatch();
}

static bool isValidUintRange(const QString &value, quint64 max = 0xFFFFFFFFu)
{
    const QRegularExpressionMatch m = kUintRangeRegex.match(value);
    if (!m.hasMatch()) {
        return false;
    }
    const quint64 lo = m.captured(1).toULongLong();
    const quint64 hi = m.hasCaptured(2) ? m.captured(2).toULongLong() : lo;
    return lo <= hi && hi <= max;
}

// Normalized to "true"/"false" because the amneziawg-go UAPI parses with Go's strconv.ParseBool,
// which rejects the awg-quick "on"/"off" spelling.
static QString normalizeBool(const QString &value)
{
    const QString v = value.toLower();
    if (v == "on" || v == "true" || v == "1" || v == "yes") {
        return QStringLiteral("true");
    }
    if (v == "off" || v == "false" || v == "0" || v == "no") {
        return QStringLiteral("false");
    }
    return QString();
}

// Join a QSettings multi-value list, trimming each element. The stored value must equal the form
// that validate() checks (which trims each element), so a leading/trailing-whitespace element can't
// pass validation while an untrimmed form is what later reaches the generated config.
static QString joinTrimmed(const QStringList &list)
{
    QStringList out;
    out.reserve(list.size());
    for (const QString &s : list) {
        out.append(s.trimmed());
    }
    return out.join(",");
}

CUSTOM_CONFIG_TYPE WireguardCustomConfig::type() const
{
    return CUSTOM_CONFIG_WIREGUARD;
}

QString WireguardCustomConfig::name() const
{
    return name_;
}

QString WireguardCustomConfig::nick() const
{
    return nick_;
}

QString WireguardCustomConfig::filename() const
{
    return filename_;
}

QStringList WireguardCustomConfig::hostnames() const
{
    QStringList list;
    if (!endpointHostname_.isEmpty())
        list << endpointHostname_;
    return list;
}

bool WireguardCustomConfig::isAllowFirewallAfterConnection() const
{
    return isAllowFirewallAfterConnection_;
}

bool WireguardCustomConfig::isCorrect() const
{
    return errMessage_.isEmpty();
}

QString WireguardCustomConfig::getErrorForIncorrect() const
{
    return errMessage_;
}

QSharedPointer<WireGuardConfig> WireguardCustomConfig::getWireGuardConfig(const QString &endpointIp) const
{
    auto *config = new WireGuardConfig(privateKey_, ipAddress_, dnsAddress_, publicKey_,
                                       presharedKey_, endpointIp + endpointPort_, allowedIps_);
    if (obfuscationParams_.isValid()) {
        config->setAmneziawgParam(obfuscationParams_);
    }
    if (!persistentKeepalive_.isEmpty()) {
        config->setPeerPersistentKeepalive(persistentKeepalive_);
    }
    config->setDisconnectOnStaleHandshake(false);
    return QSharedPointer<WireGuardConfig>(config);
}

// static
ICustomConfig *WireguardCustomConfig::makeFromFile(const QString &filepath)
{
    WireguardCustomConfig *config = new WireguardCustomConfig();
    QFileInfo fi(filepath);
    config->name_ = fi.completeBaseName();
    config->filename_ = fi.fileName();

    // Reject oversized files before parsing (DoS prevention)
    if (fi.size() > kMaxConfigFileSize) {
        config->errMessage_ = QObject::tr("Config file too large");
        return config;
    }

    config->loadFromFile(filepath);  // here the config can change to incorrect
    config->validate();
    return config;
}

void WireguardCustomConfig::loadFromFile(const QString &filepath)
{
    QSettings file(filepath, QSettings::IniFormat);
    switch (file.status()) {
    default:
    case QSettings::AccessError:
        qDebug(LOG_CUSTOM_OVPN) << "Failed to open file" << Utils::cleanSensitiveInfo(filepath);
        errMessage_ = QObject::tr("Failed to open file");
        return;
    case QSettings::FormatError:
        qDebug(LOG_CUSTOM_OVPN) << "Failed to parse file" << Utils::cleanSensitiveInfo(filepath);
        errMessage_ = QObject::tr("Invalid config format");
        return;
    case QSettings::NoError:
        // File exists and format is correct.
        break;
    }

    auto groups = file.childGroups();
    if (groups.indexOf("Interface") < 0) {
        errMessage_ = QObject::tr("Missing \"Interface\" section");
        return;
    }
    file.beginGroup("Interface");
    privateKey_ = file.value("PrivateKey").toString();
    ipAddress_ = joinTrimmed(file.value("Address").toStringList());
    dnsAddress_ = joinTrimmed(file.value("DNS").toStringList());

    obfuscationParams_ = api_responses::AmneziawgUnblockParam();
    if (file.contains("Jc"))
        obfuscationParams_.jc = file.value("Jc").toInt();
    if (file.contains("Jmin"))
        obfuscationParams_.jmin = file.value("Jmin").toInt();
    if (file.contains("Jmax"))
        obfuscationParams_.jmax = file.value("Jmax").toInt();
    if (file.contains("S1"))
        obfuscationParams_.s1 = file.value("S1").toInt();
    if (file.contains("S2"))
        obfuscationParams_.s2 = file.value("S2").toInt();
    if (file.contains("S3"))
        obfuscationParams_.s3 = file.value("S3").toInt();
    if (file.contains("S4"))
        obfuscationParams_.s4 = file.value("S4").toInt();
    if (file.contains("H1"))
        obfuscationParams_.h1 = file.value("H1").toString();
    if (file.contains("H2"))
        obfuscationParams_.h2 = file.value("H2").toString();
    if (file.contains("H3"))
        obfuscationParams_.h3 = file.value("H3").toString();
    if (file.contains("H4"))
        obfuscationParams_.h4 = file.value("H4").toString();
    if (file.contains("I1"))
        obfuscationParams_.iValues.append(file.value("I1").toString());
    if (file.contains("I2"))
        obfuscationParams_.iValues.append(file.value("I2").toString());
    if (file.contains("I3"))
        obfuscationParams_.iValues.append(file.value("I3").toString());
    if (file.contains("I4"))
        obfuscationParams_.iValues.append(file.value("I4").toString());
    if (file.contains("I5"))
        obfuscationParams_.iValues.append(file.value("I5").toString());
    obfuscationParams_.headerProtectionKey = file.value("HeaderProtectionKey").toString().trimmed();
    obfuscationParams_.contentPaddingAddition = file.value("ContentPaddingAddition").toString().trimmed();
    obfuscationParams_.rekeyAfterTime = file.value("RekeyAfterTime").toString().trimmed();
    obfuscationParams_.rekeyTimeout = file.value("RekeyTimeout").toString().trimmed();
    obfuscationParams_.rejectAfterTime = file.value("RejectAfterTime").toString().trimmed();
    obfuscationParams_.keepaliveTimeout = file.value("KeepaliveTimeout").toString().trimmed();
    obfuscationParams_.maxHandshakeAttempts = file.value("MaxHandshakeAttempts").toString().trimmed();
    obfuscationParams_.randomTrailers = file.value("RandomTrailers").toString().trimmed();
    obfuscationParams_.disableCookies = file.value("DisableCookies").toString().trimmed();

    file.endGroup();

    if (groups.indexOf("Peer") < 0) {
        errMessage_ = QObject::tr("Missing \"Peer\" section");
        return;
    }
    file.beginGroup("Peer");
    publicKey_ = file.value("PublicKey").toString();
    presharedKey_ = file.value("PresharedKey").toString();
    allowedIps_ = joinTrimmed(file.value("AllowedIPs", "0.0.0.0/0").toStringList());
    persistentKeepalive_ = file.value("PersistentKeepalive").toString().trimmed();
    if (!allowedIps_.contains("/0"))
        isAllowFirewallAfterConnection_ = false;
    QStringList endpointParts = file.value("Endpoint").toString().split(":");
    endpointHostname_ = endpointParts[0];
    endpointPort_.clear();
    endpointPortNumber_ = 0;
    if (endpointParts.size() > 1) {
        endpointPortNumber_ = endpointParts[1].toUInt();
        if (endpointPortNumber_ > 0)
            endpointPort_ = QString(":%1").arg(endpointPortNumber_);
    }
    file.endGroup();
    nick_ = endpointHostname_;
}

void WireguardCustomConfig::validate()
{
    // If already incorrect, skip further validation.
    if (!errMessage_.isEmpty())
        return;

    // --- Required field presence checks ---
    if (privateKey_.isEmpty()) {
        errMessage_ = QObject::tr("Missing \"PrivateKey\" in the \"Interface\" section");
        return;
    }
    if (ipAddress_.isEmpty()) {
        errMessage_ = QObject::tr("Missing \"Address\" in the \"Interface\" section");
        return;
    }
    if (dnsAddress_.isEmpty()) {
        errMessage_ = QObject::tr("Missing \"DNS\" in the \"Interface\" section");
        return;
    }
    if (publicKey_.isEmpty()) {
        errMessage_ = QObject::tr("Missing \"PublicKey\" in the \"Peer\" section");
        return;
    }
    if (endpointHostname_.isEmpty()) {
        errMessage_ = QObject::tr("Missing \"Endpoint\" in the \"Peer\" section");
        return;
    }

    // --- Key format validation (32 bytes = 44 chars base64) ---
    if (!isValidWgKey(privateKey_)) {
        errMessage_ = QObject::tr("Invalid \"PrivateKey\" format");
        return;
    }
    if (!isValidWgKey(publicKey_)) {
        errMessage_ = QObject::tr("Invalid \"PublicKey\" format");
        return;
    }
    if (!presharedKey_.isEmpty() && !isValidWgKey(presharedKey_)) {
        errMessage_ = QObject::tr("Invalid \"PresharedKey\" format");
        return;
    }

    // --- IP/CIDR validation (Address, AllowedIPs) ---
    for (const QString &addr : ipAddress_.split(',', Qt::SkipEmptyParts)) {
        if (!NetworkingValidation::isIpCidr(addr.trimmed())) {
            errMessage_ = QObject::tr("Invalid \"Address\" value: %1").arg(addr.trimmed());
            return;
        }
    }
    for (const QString &ip : allowedIps_.split(',', Qt::SkipEmptyParts)) {
        if (!NetworkingValidation::isIpCidr(ip.trimmed())) {
            errMessage_ = QObject::tr("Invalid \"AllowedIPs\" value: %1").arg(ip.trimmed());
            return;
        }
    }

    // --- DNS validation ---
    for (const QString &dns : dnsAddress_.split(',', Qt::SkipEmptyParts)) {
        if (!NetworkingValidation::isIp(dns.trimmed())) {
            errMessage_ = QObject::tr("Invalid \"DNS\" value: %1").arg(dns.trimmed());
            return;
        }
    }

    // --- Endpoint validation ---
    if (endpointPortNumber_ == 0 || endpointPortNumber_ > 65535) {
        errMessage_ = QObject::tr("Invalid endpoint port");
        return;
    }

    // --- AmneziaWG parameter validation ---
    if (obfuscationParams_.isValid()) {
        // H1-H4: must be numeric uint32 strings when present
        for (const QString &h : {obfuscationParams_.h1, obfuscationParams_.h2,
                                  obfuscationParams_.h3, obfuscationParams_.h4}) {
            if (!h.isEmpty() && !kHeaderValueRegex.match(h).hasMatch()) {
                errMessage_ = QObject::tr("Invalid AmneziaWG header parameter");
                return;
            }
        }
        // I1-I5: length limit (hex-encoded init packet data, bounded by MTU)
        // and printable-ASCII content limit (blocks downstream config / UAPI line
        // injection via embedded newlines or control characters).
        // I-values are emitted into newline-delimited wireguard.conf and UAPI streams
        // downstream. QSettings decodes INI escapes (\n, \xHH, etc.) into literal bytes,
        // so a malicious imported config could otherwise smuggle a newline through and
        // inject extra config lines or UAPI records. Restrict to printable ASCII.
        for (const QString &iVal : obfuscationParams_.iValues) {
            if (iVal.length() > kMaxIValueLength) {
                errMessage_ = QObject::tr("AmneziaWG init parameter too large");
                return;
            }
            if (!NetworkingValidation::isPrintableSingleLineAscii(iVal)) {
                errMessage_ = QObject::tr("AmneziaWG init parameter contains invalid characters");
                return;
            }
        }

        const QString &hpk = obfuscationParams_.headerProtectionKey;
        if (!hpk.isEmpty() && !isValidWgKey(hpk)) {
            errMessage_ = QObject::tr("Invalid AmneziaWG parameter: %1").arg("HeaderProtectionKey");
            return;
        }
        if (!hpk.isEmpty()) {
            const std::pair<const char *, int> paddings[] = {
                {"S1", obfuscationParams_.s1},
                {"S2", obfuscationParams_.s2},
                {"S3", obfuscationParams_.s3},
                {"S4", obfuscationParams_.s4},
            };
            for (const auto &[name, padding] : paddings) {
                if (padding < kMinHeaderProtectionPadding) {
                    errMessage_ = QObject::tr("Invalid AmneziaWG parameter: %1").arg(name);
                    return;
                }
            }
        }

        const std::pair<const char *, const QString *> ranges[] = {
            {"ContentPaddingAddition", &obfuscationParams_.contentPaddingAddition},
            {"RekeyAfterTime", &obfuscationParams_.rekeyAfterTime},
            {"RekeyTimeout", &obfuscationParams_.rekeyTimeout},
            {"RejectAfterTime", &obfuscationParams_.rejectAfterTime},
            {"KeepaliveTimeout", &obfuscationParams_.keepaliveTimeout},
            {"MaxHandshakeAttempts", &obfuscationParams_.maxHandshakeAttempts},
        };
        for (const auto &[name, value] : ranges) {
            if (!value->isEmpty() && !isValidUintRange(*value)) {
                errMessage_ = QObject::tr("Invalid AmneziaWG parameter: %1").arg(name);
                return;
            }
        }

        const std::pair<const char *, QString *> bools[] = {
            {"RandomTrailers", &obfuscationParams_.randomTrailers},
            {"DisableCookies", &obfuscationParams_.disableCookies},
        };
        for (const auto &[name, value] : bools) {
            if (value->isEmpty()) {
                continue;
            }
            *value = normalizeBool(*value);
            if (value->isEmpty()) {
                errMessage_ = QObject::tr("Invalid AmneziaWG parameter: %1").arg(name);
                return;
            }
        }
    }

    // --- PersistentKeepalive: "N", "N-M" (AmneziaWG only) or "off" ---
    if (persistentKeepalive_.compare("off", Qt::CaseInsensitive) == 0) {
        persistentKeepalive_ = "0";
    }
    if (!persistentKeepalive_.isEmpty()) {
        const bool isAmneziawg = obfuscationParams_.isValid();
        const bool isRange = persistentKeepalive_.contains('-');
        const quint64 max = isAmneziawg ? 0xFFFFFFFFu : kMaxVanillaPersistentKeepalive;
        if (!isValidUintRange(persistentKeepalive_, max) || (isRange && !isAmneziawg)) {
            errMessage_ = QObject::tr("Invalid \"PersistentKeepalive\" value");
            return;
        }
    }
}

} //namespace customconfigs

