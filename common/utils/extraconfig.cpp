#include "extraconfig.h"

#include <QFile>
#include <QStandardPaths>
#include "../utils/logger.h"

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

const QString WS_STAGING_STR    = WS_PREFIX + "staging";

void ExtraConfig::writeConfig(const QString &cfg)
{
    QMutexLocker locker(&mutex_);
    QFile file(path_);
    if (cfg.isEmpty())
    {
        file.remove();
    }
    else
    {
        if (file.open(QIODeviceBase::WriteOnly))
        {
            file.resize(0);
            file.write(cfg.toLocal8Bit());
            file.close();
        }
    }
}


QString ExtraConfig::getExtraConfig(bool bWithLog)
{
    QMutexLocker locker(&mutex_);
    QFile fileExtra(path_);
    if (fileExtra.open(QIODeviceBase::ReadOnly))
    {
        QByteArray extraArr = fileExtra.readAll();
        if (bWithLog)
        {
            qCDebug(LOG_BASIC) << "Found extra config file:" << path_;
            qCDebug(LOG_BASIC) << "Extra options:" << extraArr;
        }

        fileExtra.close();
        return extraArr;
    }
    else
    {
        return "";
    }
}

QString ExtraConfig::getExtraConfigForOpenVpn()
{
    QMutexLocker locker(&mutex_);
    QString result;
    const QStringList strs = getExtraConfig().split("\n");
    for (const QString &line: strs)
    {
        if (isLegalOpenVpnCommand(line))
            result += line + "\n";
    }
    return result;
}

QString ExtraConfig::getExtraConfigForIkev2()
{
    QMutexLocker locker(&mutex_);
    QString result;
    const QString strExtraConfig = getExtraConfig();
    const QStringList strs = strExtraConfig.split("\n");
    for (const QString &line : strs)
    {
        QString lineTrimmed = line.trimmed();
        if (lineTrimmed.startsWith("--ikev2", Qt::CaseInsensitive))
        {
            result += line + "\n";
        }
    }
    return result;
}

bool ExtraConfig::isUseIkev2Compression()
{
    QMutexLocker locker(&mutex_);
    QString config = getExtraConfigForIkev2();
    return config.contains("--ikev2-compression", Qt::CaseInsensitive);
}

QString ExtraConfig::getRemoteIpFromExtraConfig()
{
    QMutexLocker locker(&mutex_);
    const QString strExtraConfig = getExtraConfig(false);
    const QStringList strs = strExtraConfig.split("\n");
    for (const QString &line : strs)
    {
        if (line.contains("remote", Qt::CaseInsensitive))
        {
            QStringList words = line.split(" ");
            if (words.size() == 2)
            {
                if (words[0].trimmed().compare("remote", Qt::CaseInsensitive) == 0)
                {
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
    if (indExtra == -1)
    {
        return ovpnData;
    }
    QString verbString = strExtraConfig.mid(indExtra, match.capturedLength());

    QString strOvpn = ovpnData;

    int ind = strOvpn.indexOf(regExp_, 0, &match);
    if (ind == -1)
    {
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

bool ExtraConfig::getOverrideUpdateChannelToInternal()
{
    return getFlagFromExtraConfigLines(WS_UPDATE_CHANNEL_INTERNAL);
}

bool ExtraConfig::getIsStaging()
{
    return getFlagFromExtraConfigLines(WS_STAGING_STR);
}

int ExtraConfig::getIntFromLineWithString(const QString &line, const QString &str, bool &success)
{
    int endOfId = line.indexOf(str, Qt::CaseInsensitive) + str.length();
    int equals = line.indexOf("=", endOfId, Qt::CaseInsensitive)+1;

    int result = 0;
    if (equals != -1)
    {
        QString afterEquals = line.mid(equals).trimmed();
        result = afterEquals.toInt(&success);
    }

    return result;
}

int ExtraConfig::getIntFromExtraConfigLines(const QString &variableName, bool &success)
{
    success = false;

    const QString strExtraConfig = getExtraConfig();
    const QStringList strs = strExtraConfig.split("\n");

    for (const QString &line : strs)
    {
        QString lineTrimmed = line.trimmed();

        if (lineTrimmed.startsWith(variableName, Qt::CaseInsensitive))
        {
            int result = getIntFromLineWithString(lineTrimmed, variableName, success);

            if (success)
            {
                return result;
            }
        }
    }

    return 0;
}

bool ExtraConfig::getFlagFromExtraConfigLines(const QString &flagName)
{
    const QString strExtraConfig = getExtraConfig();
    const QStringList strs = strExtraConfig.split("\n");

    for (const QString &line : strs)
    {
        QString lineTrimmed = line.trimmed();
        if (lineTrimmed.startsWith(flagName, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;

}

bool ExtraConfig::isLegalOpenVpnCommand(const QString &command) const
{
    const QString trimmed_command = command.trimmed();
    if (trimmed_command.isEmpty())
        return false;

    // Filter out IKEv2, mtu, and tunnel test related commands.
    if (trimmed_command.startsWith("--ikev2", Qt::CaseInsensitive)
        || trimmed_command.startsWith(WS_PREFIX, Qt::CaseInsensitive))
        return false;

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

ExtraConfig::ExtraConfig() : mutex_(QRecursiveMutex()),
                             path_(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                   + "/windscribe_extra.conf"),
                             regExp_("(?m)^(?i)(verb)(\\s+)(\\d+$)")
{
}
