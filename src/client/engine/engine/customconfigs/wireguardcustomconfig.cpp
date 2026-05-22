#include "wireguardcustomconfig.h"
#include "utils/log/categories.h"
#include "utils/log/clean_sensitive_info.h"

#include <QFileInfo>
#include <QHostAddress>
#include <QRegularExpression>
#include <QSettings>

namespace customconfigs {

// Maximum file size for a WireGuard .conf (no legitimate config approaches this).
static constexpr qint64 kMaxConfigFileSize = 64 * 1024;

// WireGuard keys are 32 bytes, base64-encoded = 44 characters (including trailing '=').
static const QRegularExpression kBase64KeyRegex(QStringLiteral("^[A-Za-z0-9+/]{43}=$"));

// H1-H4 are either a single uint32 decimal string or a range "x-y" of two uint32 decimal strings.
// See: https://github.com/amnezia-vpn/amneziawg-go (message paddings).
static const QRegularExpression kHeaderValueRegex(QStringLiteral("^[0-9]{1,10}(-[0-9]{1,10})?$"));

// I1-I5 are tag-syntax init-packet specs: <b 0xHEX>, <r N>, <rd N>, <rc N>, <t>
// (see amneziawg-go README). MTU bounds the expanded payload to ~1500 bytes;
// 4096 leaves comfortable margin for the literal string form.
static constexpr int kMaxIValueLength = 4096;

static bool isValidWgKey(const QString &key)
{
    return kBase64KeyRegex.match(key).hasMatch();
}

static bool isValidIpCidr(const QString &str)
{
    QStringList parts = str.split('/');
    if (parts.isEmpty() || parts.size() > 2)
        return false;

    QHostAddress addr;
    if (!addr.setAddress(parts[0]))
        return false;

    if (parts.size() == 2) {
        bool ok = false;
        int cidr = parts[1].toInt(&ok);
        if (!ok || cidr < 0)
            return false;
        int maxCidr = (addr.protocol() == QAbstractSocket::IPv4Protocol) ? 32 : 128;
        if (cidr > maxCidr)
            return false;
    }

    return true;
}

static bool isValidIpAddress(const QString &str)
{
    QHostAddress addr;
    return addr.setAddress(str);
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
    ipAddress_ = WireGuardConfig::stripIpv6Address(file.value("Address").toStringList());
    dnsAddress_ = WireGuardConfig::stripIpv6Address(file.value("DNS").toStringList());

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

    file.endGroup();

    if (groups.indexOf("Peer") < 0) {
        errMessage_ = QObject::tr("Missing \"Peer\" section");
        return;
    }
    file.beginGroup("Peer");
    publicKey_ = file.value("PublicKey").toString();
    presharedKey_ = file.value("PresharedKey").toString();
    allowedIps_ =
        WireGuardConfig::stripIpv6Address(file.value("AllowedIPs", "0.0.0.0/0").toStringList());
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
        if (!isValidIpCidr(addr.trimmed())) {
            errMessage_ = QObject::tr("Invalid \"Address\" value: %1").arg(addr.trimmed());
            return;
        }
    }
    for (const QString &ip : allowedIps_.split(',', Qt::SkipEmptyParts)) {
        if (!isValidIpCidr(ip.trimmed())) {
            errMessage_ = QObject::tr("Invalid \"AllowedIPs\" value: %1").arg(ip.trimmed());
            return;
        }
    }

    // --- DNS validation ---
    for (const QString &dns : dnsAddress_.split(',', Qt::SkipEmptyParts)) {
        if (!isValidIpAddress(dns.trimmed())) {
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
        for (const QString &iVal : obfuscationParams_.iValues) {
            if (iVal.length() > kMaxIValueLength) {
                errMessage_ = QObject::tr("AmneziaWG init parameter too large");
                return;
            }
        }
    }
}

} //namespace customconfigs

