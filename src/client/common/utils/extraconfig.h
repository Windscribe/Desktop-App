#pragma once

#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>

#include <optional>

// for work with windscribe_extra.conf file (additional parameters), thread-safe access
// all ikev2 params will be prefixed with --ikev2, so you don't use them when using OpenVPN, and vise versa
// ikev2 options:
// --ikev2-compression - enable ikev2 compression for Windows (RASEO_IpHeaderCompression and RASEO_SwCompression flags)
class ExtraConfig : public QObject
{
    Q_OBJECT
public:
    static ExtraConfig &instance()
    {
        static ExtraConfig s;
        return s;
    }

    void logExtraConfig();
    void writeConfig(const QString &cfg, bool bWithLog = true);

    QString getExtraConfig();
    QString getExtraConfigForOpenVpn();
    QString getExtraConfigForIkev2();
    bool isUseIkev2Compression();
    QString getRemoteIpFromExtraConfig();

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

    bool getWireGuardVerboseLogging();
    bool getWireGuardUdpStuffing();
    bool getNoPings();

    std::optional<QString> serverlistCountryOverride();
    bool serverListIgnoreCountryOverride();
    bool haveServerListCountryOverride();

    bool useOpenVpnDCO();

    QString apiRootOverride();
    QString assetsRootOverride();

    void fromJson(const QJsonObject &json);
    QJsonObject toJson();

private slots:
    void onFileChanged();
    void onDirectoryChanged(const QString &path);

private:
    ExtraConfig();

    QRecursiveMutex mutex_;
    QString path_;
    QString detectedIp_;

    // File watching
    QFileSystemWatcher* fileWatcher_;
    bool fileExists_;
    QStringList configLines_;
    QHash<QString, QString> parsedValues_;

    static const inline QString kJsonFileContentsProp = "fileContents";

    void parseConfigFile();
    void updateFileWatchingState();

    int getInt(const QString &variableName, bool &success);
    bool getFlag(const QString &flagName);
    QString getString(const QString &variableName);

    bool isLegalOpenVpnCommand(const QString &command) const;

    QStringList extraConfigEntries();
    std::optional<QString> getValue(const QString& key);
};
