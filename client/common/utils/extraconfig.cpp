#include "extraconfig.h"

#include <QFile>
#include <QStandardPaths>

#include "../utils/log/categories.h"

#if defined (Q_OS_WIN)
#include "types/global_consts.h"
#include "utils/winutils.h"
#endif

// non-filtered windscribe-specific advanced settings will cause error when connecting with openvpn
// configs prefixed with "ws-" will be ignored by openvpn profile
const QString WS_PREFIX = "ws-";
const QString WS_MTU_OFFSET_IKEV_STR     = WS_PREFIX + "mtu-offset-ikev2";
const QString WS_MTU_OFFSET_OPENVPN_STR  = WS_PREFIX + "mtu-offset-openvpn";
const QString WS_MTU_OFFSET_WG_STR       = WS_PREFIX + "mtu-offset-wg";
const QString WS_UPDATE_CHANNEL_INTERNAL = WS_PREFIX + "override-update-channel-internal";

const QString WS_TT_START_DELAY_STR = WS_PREFIX + "tunnel-test-start-delay";
const QString WS_TT_TIMEOUT_STR     = WS_PREFIX + "tunnel-test-timeout";
const QString WS_TT_RETRY_DELAY_STR = WS_PREFIX + "tunnel-test-retry-delay";
const QString WS_TT_ATTEMPTS_STR    = WS_PREFIX + "tunnel-test-attempts";
const QString WS_TT_NO_ERROR_STR    = WS_PREFIX + "tunnel-test-no-error";

const QString WS_STAGING_STR = WS_PREFIX + "staging";

const QString WS_LOG_API_RESPONSE = WS_PREFIX + "log-api-response";
const QString WS_WG_VERBOSE_LOGGING = WS_PREFIX + "wireguard-verbose-logging";
const QString WS_SCREEN_TRANSITION_HOTKEYS = WS_PREFIX + "screen-transition-hotkeys";
const QString WS_USE_ICMP_PINGS = WS_PREFIX + "use-icmp-pings";

const QString WS_STEALTH_EXTRA_TLS_PADDING = WS_PREFIX + "stealth-extra-tls-padding";
const QString WS_API_EXTRA_TLS_PADDING = WS_PREFIX + "api-extra-tls-padding";
const QString WS_WG_UDP_STUFFING = WS_PREFIX + "wireguard-udp-stuffing";

const QString WS_SERVERLIST_COUNTRY_OVERRIDE = WS_PREFIX + "serverlist-country-override";

const QString WS_USE_OPENVPN_WINTUN = WS_PREFIX + "use-openvpn-wintun";
const QString WS_USE_PUBLIC_NETWORK_CATEGORY = WS_PREFIX + "use-public-network-category";

const QString WS_LOG_CTRLD = WS_PREFIX + "log-ctrld";
const QString WS_LOG_PINGS = WS_PREFIX + "log-pings";
const QString WS_LOG_SPLITTUNNELEXTENSION = WS_PREFIX + "log-splittunnelextension";
const QString WS_NO_PINGS = WS_PREFIX + "no-pings";


void ExtraConfig::writeConfig(const QString &cfg, bool bWithLog)
{
    QMutexLocker locker(&mutex_);
    QFile file(path_);
    if (cfg.isEmpty()) {
        file.remove();
        return;
    }

    if (file.open(QIODevice::WriteOnly)) {
        file.resize(0);
        file.write(cfg.toLocal8Bit());
        file.close();

        if (bWithLog) {
            qCDebug(LOG_BASIC) << "Wrote extra config file:" << path_;
            qCDebug(LOG_BASIC) << "Extra options:" << cfg.toLocal8Bit();
        }
    }
}

QString ExtraConfig::getExtraConfig(bool bWithLog)
{
    QMutexLocker locker(&mutex_);
    QFile fileExtra(path_);
    if (fileExtra.exists() && fileExtra.open(QIODevice::ReadOnly)) {
        QByteArray extraArr = fileExtra.readAll();
        fileExtra.close();
        if (bWithLog) {
            qCDebug(LOG_BASIC) << "Found extra config file";
            qCDebug(LOG_BASIC) << "Extra options:" << extraArr;
        }
        return extraArr;
    }

    return "";
}

QStringList ExtraConfig::extraConfigEntries()
{
    const QString config = getExtraConfig();
    return config.split("\n");
}

std::optional<QString> ExtraConfig::getValue(const QString& key)
{
    const QStringList strs = extraConfigEntries();
    for (const QString &line : strs) {
        QString lineTrimmed = line.trimmed();
        if (lineTrimmed.startsWith(key, Qt::CaseInsensitive)) {
            QString value = line.mid(key.length());
            return value.remove('=').trimmed();
        }
    }

    return std::nullopt;
}

QString ExtraConfig::getExtraConfigForOpenVpn()
{
    QString result;
    const QStringList strs = extraConfigEntries();
    for (const QString &line: strs) {
        if (isLegalOpenVpnCommand(line))
            result += line + "\n";
    }
    return result;
}

QString ExtraConfig::getExtraConfigForIkev2()
{
    QString result;
    const QStringList strs = extraConfigEntries();
    for (const QString &line : strs) {
        QString lineTrimmed = line.trimmed();
        if (lineTrimmed.startsWith("--ikev2", Qt::CaseInsensitive)) {
            result += line + "\n";
        }
    }
    return result;
}

bool ExtraConfig::isUseIkev2Compression()
{
    QString config = getExtraConfigForIkev2();
    return config.contains("--ikev2-compression", Qt::CaseInsensitive);
}

QString ExtraConfig::getRemoteIpFromExtraConfig()
{
    const QStringList strs = extraConfigEntries();
    for (const QString &line : strs) {
        if (line.contains("remote", Qt::CaseInsensitive)) {
            QStringList words = line.split(" ");
            if (words.size() == 2) {
                if (words[0].trimmed().compare("remote", Qt::CaseInsensitive) == 0) {
                    return words[1].trimmed();
                }
            }
        }
    }
    return "";
}

QString ExtraConfig::modifyVerbParameter(const QString &ovpnData, QString &strExtraConfig)
{
    QRegularExpressionMatch match;
    int indExtra = strExtraConfig.indexOf(regExp_, 0, &match);
    if (indExtra == -1) {
        return ovpnData;
    }
    QString verbString = strExtraConfig.mid(indExtra, match.capturedLength());

    QString strOvpn = ovpnData;

    int ind = strOvpn.indexOf(regExp_, 0, &match);
    if (ind == -1) {
        return ovpnData;
    }

    strOvpn.replace(regExp_, verbString);
    strExtraConfig.remove(indExtra, match.capturedLength());

    return strOvpn;
}

int ExtraConfig::getMtuOffsetIkev2(bool &success)
{
    return getIntFromExtraConfigLines(WS_MTU_OFFSET_IKEV_STR, success);
}

int ExtraConfig::getMtuOffsetOpenVpn(bool &success)
{
    return getIntFromExtraConfigLines(WS_MTU_OFFSET_OPENVPN_STR, success);
}

int ExtraConfig::getMtuOffsetWireguard(bool &success)
{
    return getIntFromExtraConfigLines(WS_MTU_OFFSET_WG_STR, success);
}

int ExtraConfig::getTunnelTestStartDelay(bool &success)
{
    int delay = getIntFromExtraConfigLines(WS_TT_START_DELAY_STR, success);
    if (success && delay < 0) {
        delay = 0;
    }

    return delay;
}

int ExtraConfig::getTunnelTestTimeout(bool &success)
{
    int timeout = getIntFromExtraConfigLines(WS_TT_TIMEOUT_STR, success);
    if (success && timeout < 0) {
        timeout = 0;
    }

    return timeout;
}

int ExtraConfig::getTunnelTestRetryDelay(bool &success)
{
    int delay = getIntFromExtraConfigLines(WS_TT_RETRY_DELAY_STR, success);
    if (success && delay < 0) {
        delay = 0;
    }

    return delay;
}

int ExtraConfig::getTunnelTestAttempts(bool &success)
{
    int attempts = getIntFromExtraConfigLines(WS_TT_ATTEMPTS_STR, success);
    if (success && attempts < 0) {
        attempts = 0;
    }

    return attempts;
}

bool ExtraConfig::getIsTunnelTestNoError()
{
    return getFlagFromExtraConfigLines(WS_TT_NO_ERROR_STR);
}

bool ExtraConfig::getOverrideUpdateChannelToInternal()
{
    return getFlagFromExtraConfigLines(WS_UPDATE_CHANNEL_INTERNAL);
}

bool ExtraConfig::getIsStaging()
{
    return getFlagFromExtraConfigLines(WS_STAGING_STR);
}

bool ExtraConfig::getLogAPIResponse()
{
    return getFlagFromExtraConfigLines(WS_LOG_API_RESPONSE);
}

bool ExtraConfig::getLogCtrld()
{
    return getFlagFromExtraConfigLines(WS_LOG_CTRLD);
}

bool ExtraConfig::getLogPings()
{
    return getFlagFromExtraConfigLines(WS_LOG_PINGS);
}

bool ExtraConfig::getLogSplitTunnelExtension()
{
    return getFlagFromExtraConfigLines(WS_LOG_SPLITTUNNELEXTENSION);
}

bool ExtraConfig::getWireGuardVerboseLogging()
{
    return getFlagFromExtraConfigLines(WS_WG_VERBOSE_LOGGING);
}

bool ExtraConfig::getUsingScreenTransitionHotkeys()
{
    return getFlagFromExtraConfigLines(WS_SCREEN_TRANSITION_HOTKEYS);
}

bool ExtraConfig::getUseICMPPings()
{
    return getFlagFromExtraConfigLines(WS_USE_ICMP_PINGS);
}

bool ExtraConfig::getStealthExtraTLSPadding()
{
    return getFlagFromExtraConfigLines(WS_STEALTH_EXTRA_TLS_PADDING);
}

bool ExtraConfig::getAPIExtraTLSPadding()
{
    return getFlagFromExtraConfigLines(WS_API_EXTRA_TLS_PADDING);
}

bool ExtraConfig::getWireGuardUdpStuffing()
{
    return getFlagFromExtraConfigLines(WS_WG_UDP_STUFFING);
}

bool ExtraConfig::getNoPings()
{
    return getFlagFromExtraConfigLines(WS_NO_PINGS);
}

std::optional<QString> ExtraConfig::serverlistCountryOverride()
{
    auto value = getValue(WS_SERVERLIST_COUNTRY_OVERRIDE);
    if (value.has_value() && !value.value().isEmpty()) {
        return value;
    }

    return std::nullopt;
}

bool ExtraConfig::serverListIgnoreCountryOverride()
{
    auto value = serverlistCountryOverride();
    if (value.has_value() && value.value().compare("ignore", Qt::CaseInsensitive) == 0) {
        return true;
    }

    return false;
}

bool ExtraConfig::haveServerListCountryOverride()
{
    auto value = serverlistCountryOverride();
    if (value.has_value() && value.value().compare("ignore", Qt::CaseInsensitive) != 0) {
        return true;
    }

    return false;
}

int ExtraConfig::getIntFromExtraConfigLines(const QString &variableName, bool &success)
{
    auto value = getValue(variableName);
    if (value.has_value()) {
        return value->toInt(&success);
    }

    success = false;
    return 0;
}

bool ExtraConfig::getFlagFromExtraConfigLines(const QString &flagName)
{
    auto value = getValue(flagName);
    return value.has_value();
}

bool ExtraConfig::isLegalOpenVpnCommand(const QString &command) const
{
    const QString trimmed_command = command.trimmed();
    if (trimmed_command.isEmpty())
        return false;

    // Filter out IKEv2, mtu, and tunnel test related commands.
    if (trimmed_command.startsWith("--ikev2", Qt::CaseInsensitive) ||
        trimmed_command.startsWith(WS_PREFIX, Qt::CaseInsensitive)) {
        return false;
    }

    // Filter out potentially malicious commands.
    const char *kUnsafeCommands[] = {
        "up", "down", "ipchange", "route-up", "route-pre-down", "auth-user-pass-verify",
        "client-connect", "client-disconnect", "learn-address", "tls-verify", "log", "log-append",
        "tmp-dir"
    };
    const size_t kNumUnsafeCommands = sizeof(kUnsafeCommands) / sizeof(kUnsafeCommands[0]);
    for (size_t i = 0; i < kNumUnsafeCommands; ++i) {
        if (trimmed_command.startsWith(QString("%1 ").arg(kUnsafeCommands[i]), Qt::CaseInsensitive))
            return false;
    }
    return true;
}

bool ExtraConfig::useOpenVpnDCO()
{
    bool useDCO = !getFlagFromExtraConfigLines(WS_USE_OPENVPN_WINTUN);
#if defined (Q_OS_WIN)
    if (useDCO) {
        if (WinUtils::getOSBuildNumber() < kMinWindowsBuildNumberForOpenVPNDCO) {
            useDCO = false;
            qCWarning(LOG_CONNECTION) << "WARNING: OS version is not compatible with the OpenVPN DCO driver.  Windows 10 build"
                                    << kMinWindowsBuildNumberForOpenVPNDCO << "or newer is required to use this driver.";
        }
    }
#endif
    return useDCO;
}

bool ExtraConfig::usePublicNetworkCategory()
{
    return getFlagFromExtraConfigLines(WS_USE_PUBLIC_NETWORK_CATEGORY);
}

ExtraConfig::ExtraConfig() : path_(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                                   + "/windscribe_extra.conf"),
                             regExp_("(?m)^(?i)(verb)(\\s+)(\\d+$)")
{
}

void ExtraConfig::logExtraConfig()
{
    getExtraConfig(true);
}

void ExtraConfig::fromJson(const QJsonObject &json)
{
    if (json.contains(kJsonFileContentsProp) && json[kJsonFileContentsProp].isString()) {
        const auto fileContents = QByteArray::fromBase64(json[kJsonFileContentsProp].toString().toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
        if (!fileContents.isEmpty()) {
            writeConfig(fileContents, false);
        }
    }
}

QJsonObject ExtraConfig::toJson()
{
    QJsonObject json;
    const auto fileContents = getExtraConfig(false);
    if (!fileContents.isEmpty()) {
        json[kJsonFileContentsProp] = QString(fileContents.toUtf8().toBase64());
    }
    return json;
}
