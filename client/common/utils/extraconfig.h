#pragma once

#include <QJsonObject>
#include <QMutex>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <optional>

// for work with windscribe_extra.conf file (additional parameters), thread-safe access
// all ikev2 params will be prefixed with --ikev2, so you don't use them when using OpenVPN, and vise versa
// ikev2 options:
// --ikev2-compression - enable ikev2 compression for Windows (RASEO_IpHeaderCompression and RASEO_SwCompression flags)
class ExtraConfig
{
public:
    static ExtraConfig &instance()
    {
        static ExtraConfig s;
        return s;
    }

    void logExtraConfig();
    void writeConfig(const QString &cfg, bool bWithLog = true);

    QString getExtraConfig(bool bWithLog = false);
    QString getExtraConfigForOpenVpn();
    QString getExtraConfigForIkev2();
    bool isUseIkev2Compression();
    QString getRemoteIpFromExtraConfig();
    QString modifyVerbParameter(const QString &ovpnData, QString &strExtraConfig);

    int getMtuOffsetIkev2(bool &success);
    int getMtuOffsetOpenVpn(bool &success);
    int getMtuOffsetWireguard(bool &success);

    // used in Engine::addCustomRemoteIpToFirewallIfNeed and later for ikev2 connection
    void setDetectedIp(const QString &ip) { detectedIp_ = ip; }
    QString getDetectedIp() { return detectedIp_; }

    int getTunnelTestStartDelay(bool &success);
    int getTunnelTestTimeout(bool &success);
    int getTunnelTestRetryDelay(bool &success);
    int getTunnelTestAttempts(bool &success);
    bool getIsTunnelTestNoError();

    bool getOverrideUpdateChannelToInternal();
    bool getIsStaging();

    bool getLogAPIResponse();
    bool getLogCtrld();
    bool getLogPings();
    bool getLogSplitTunnelExtension();
    bool getUsingScreenTransitionHotkeys();
    bool getUseICMPPings();
    bool getStealthExtraTLSPadding();
    bool getAPIExtraTLSPadding();

    bool getWireGuardVerboseLogging();
    bool getWireGuardUdpStuffing();
    bool getNoPings();

    std::optional<QString> serverlistCountryOverride();
    bool serverListIgnoreCountryOverride();
    bool haveServerListCountryOverride();

    bool useOpenVpnDCO();
    bool usePublicNetworkCategory();

    void fromJson(const QJsonObject &json);
    QJsonObject toJson();

private:
    ExtraConfig();

    QRecursiveMutex mutex_;
    QString path_;
    QRegularExpression regExp_;
    QString detectedIp_;

    static const inline QString kJsonFileContentsProp = "fileContents";

    int getIntFromExtraConfigLines(const QString &variableName, bool &success);
    bool getFlagFromExtraConfigLines(const QString &flagName);

    bool isLegalOpenVpnCommand(const QString &command) const;

    QStringList extraConfigEntries();
    std::optional<QString> getValue(const QString& key);
};
