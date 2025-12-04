#include "extraconfig.h"

#include <QFile>
#include <QFileInfo>
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
const QString WS_WG_UDP_STUFFING = WS_PREFIX + "wireguard-udp-stuffing";

const QString WS_SERVERLIST_COUNTRY_OVERRIDE = WS_PREFIX + "serverlist-country-override";

const QString WS_USE_OPENVPN_WINTUN = WS_PREFIX + "use-openvpn-wintun";

const QString WS_LOG_CTRLD = WS_PREFIX + "log-ctrld";
const QString WS_LOG_PINGS = WS_PREFIX + "log-pings";
const QString WS_LOG_SPLITTUNNELEXTENSION = WS_PREFIX + "log-splittunnelextension";
const QString WS_NO_PINGS = WS_PREFIX + "no-pings";

const QString WS_API_ROOT_OVERRIDE = WS_PREFIX + "api-root-override";
const QString WS_ASSETS_ROOT_OVERRIDE = WS_PREFIX + "assets-root-override";

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

QString ExtraConfig::getExtraConfig()
{
    QMutexLocker locker(&mutex_);
    if (configLines_.isEmpty()) {
        return "";
    } else {
        return configLines_.join("\n");
    }
}


std::optional<QString> ExtraConfig::getValue(const QString& key)
{
    QMutexLocker locker(&mutex_);
    auto it = parsedValues_.find(key.toLower());
    if (it != parsedValues_.end() && !it.value().isEmpty()) {
        return it.value();
    }

    return std::nullopt;
}

QString ExtraConfig::getExtraConfigForOpenVpn()
{
    QString result;
    QMutexLocker locker(&mutex_);
    for (const auto &line: std::as_const(configLines_)) {
        if (isLegalOpenVpnCommand(line))
            result += line + "\n";
    }
    return result;
}

QString ExtraConfig::getExtraConfigForIkev2()
{
    QString result;
    QMutexLocker locker(&mutex_);
    for (const auto &line: std::as_const(configLines_)) {
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
    QMutexLocker locker(&mutex_);
    for (const auto &line: std::as_const(configLines_)) {
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

int ExtraConfig::getMtuOffsetIkev2(bool &success)
{
    return getInt(WS_MTU_OFFSET_IKEV_STR, success);
}

int ExtraConfig::getMtuOffsetOpenVpn(bool &success)
{
    return getInt(WS_MTU_OFFSET_OPENVPN_STR, success);
}

int ExtraConfig::getMtuOffsetWireguard(bool &success)
{
    return getInt(WS_MTU_OFFSET_WG_STR, success);
}

int ExtraConfig::getTunnelTestStartDelay(bool &success)
{
    int delay = getInt(WS_TT_START_DELAY_STR, success);
    if (success && delay < 0) {
        delay = 0;
    }

    return delay;
}

int ExtraConfig::getTunnelTestTimeout(bool &success)
{
    int timeout = getInt(WS_TT_TIMEOUT_STR, success);
    if (success && timeout < 0) {
        timeout = 0;
    }

    return timeout;
}

int ExtraConfig::getTunnelTestRetryDelay(bool &success)
{
    int delay = getInt(WS_TT_RETRY_DELAY_STR, success);
    if (success && delay < 0) {
        delay = 0;
    }

    return delay;
}

int ExtraConfig::getTunnelTestAttempts(bool &success)
{
    int attempts = getInt(WS_TT_ATTEMPTS_STR, success);
    if (success && attempts < 0) {
        attempts = 0;
    }

    return attempts;
}

bool ExtraConfig::getIsTunnelTestNoError()
{
    return getFlag(WS_TT_NO_ERROR_STR);
}

bool ExtraConfig::getOverrideUpdateChannelToInternal()
{
    return getFlag(WS_UPDATE_CHANNEL_INTERNAL);
}

bool ExtraConfig::getIsStaging()
{
    return getFlag(WS_STAGING_STR);
}

bool ExtraConfig::getLogAPIResponse()
{
    return getFlag(WS_LOG_API_RESPONSE);
}

bool ExtraConfig::getLogCtrld()
{
    return getFlag(WS_LOG_CTRLD);
}

bool ExtraConfig::getLogPings()
{
    return getFlag(WS_LOG_PINGS);
}

bool ExtraConfig::getLogSplitTunnelExtension()
{
    return getFlag(WS_LOG_SPLITTUNNELEXTENSION);
}

bool ExtraConfig::getWireGuardVerboseLogging()
{
    return getFlag(WS_WG_VERBOSE_LOGGING);
}

bool ExtraConfig::getUsingScreenTransitionHotkeys()
{
    return getFlag(WS_SCREEN_TRANSITION_HOTKEYS);
}

bool ExtraConfig::getUseICMPPings()
{
    return getFlag(WS_USE_ICMP_PINGS);
}

bool ExtraConfig::getStealthExtraTLSPadding()
{
    return getFlag(WS_STEALTH_EXTRA_TLS_PADDING);
}

bool ExtraConfig::getWireGuardUdpStuffing()
{
    return getFlag(WS_WG_UDP_STUFFING);
}

bool ExtraConfig::getNoPings()
{
    return getFlag(WS_NO_PINGS);
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

int ExtraConfig::getInt(const QString &variableName, bool &success)
{
    auto value = getValue(variableName);
    if (value.has_value()) {
        return value->toInt(&success);
    }

    success = false;
    return 0;
}

bool ExtraConfig::getFlag(const QString &flagName)
{
    QMutexLocker locker(&mutex_);
    return parsedValues_.contains(flagName.toLower());
}

QString ExtraConfig::getString(const QString &variableName)
{
    auto value = getValue(variableName);
    if (value.has_value()) {
        return value.value();
    }

    return QString();
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

QString ExtraConfig::apiRootOverride()
{
    return getString(WS_API_ROOT_OVERRIDE);
}

QString ExtraConfig::assetsRootOverride()
{
    return getString(WS_ASSETS_ROOT_OVERRIDE);
}

bool ExtraConfig::useOpenVpnDCO()
{
    bool useDCO = !getFlag(WS_USE_OPENVPN_WINTUN);
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

ExtraConfig::ExtraConfig() : QObject(),
                             path_(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                                   + "/windscribe_extra.conf"),
                             fileWatcher_(nullptr),
                             fileExists_(false)
{
    fileWatcher_ = new QFileSystemWatcher(this);

    // Always watch the directory
    QString dirPath = QFileInfo(path_).absolutePath();
    fileWatcher_->addPath(dirPath);

    // Connect both signals
    connect(fileWatcher_, &QFileSystemWatcher::fileChanged, this, &ExtraConfig::onFileChanged);
    connect(fileWatcher_, &QFileSystemWatcher::directoryChanged, this, &ExtraConfig::onDirectoryChanged);

    // Initialize file watching state
    updateFileWatchingState();

    parseConfigFile();
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

void ExtraConfig::logExtraConfig()
{
    if (configLines_.isEmpty()) {
        qCDebug(LOG_BASIC) << "No extra config found";
    } else {
        qCDebug(LOG_BASIC) << "Extra config: " << configLines_.join("\n");
    }
}

QJsonObject ExtraConfig::toJson()
{
    QJsonObject json;
    QMutexLocker locker(&mutex_);
    if (!configLines_.isEmpty()) {
        QString fileContents = configLines_.join("\n");
        json[kJsonFileContentsProp] = QString(fileContents.toUtf8().toBase64());
    }
    return json;
}

void ExtraConfig::parseConfigFile()
{
    QMutexLocker locker(&mutex_);
    QFile file(path_);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        configLines_ = QString::fromLocal8Bit(data).split("\n");
    } else {
        configLines_.clear();
    }

    parsedValues_.clear();
    for (const auto &line: std::as_const(configLines_)) {
        QString lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith("#")) {
            continue;
        }

        int equalPos = lineTrimmed.indexOf('=');
        if (equalPos > 0) {
            QString key = lineTrimmed.left(equalPos).trimmed();
            QString value = lineTrimmed.mid(equalPos + 1).trimmed();
            parsedValues_[key.toLower()] = value;
        } else {
            parsedValues_[lineTrimmed.toLower()] = QString();
        }
    }
}

void ExtraConfig::onFileChanged()
{
    qCDebug(LOG_BASIC) << "Extra config file changed, re-reading";
    parseConfigFile();
    logExtraConfig();
}

void ExtraConfig::onDirectoryChanged(const QString &path)
{
    updateFileWatchingState();
}

void ExtraConfig::updateFileWatchingState()
{
    bool fileExists = QFile::exists(path_);

    if (fileExists != fileExists_) {
        if (fileExists) {
            if (!fileWatcher_->files().contains(path_)) {
                fileWatcher_->addPath(path_);
            }
            onFileChanged();
        } else {
            if (fileWatcher_->files().contains(path_)) {
                fileWatcher_->removePath(path_);
            }
            // don't need to call onFileChanged(); file deletion causes onFileChanged() to be called already.
        }
        fileExists_ = fileExists;
    }
}
