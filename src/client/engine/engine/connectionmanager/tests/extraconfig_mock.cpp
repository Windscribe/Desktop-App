#include "extraconfig_mock.h"

#include "utils/extraconfig.h"

namespace ExtraConfigMock {

QString remoteIp;
QString extraConfigForOpenVpn;
bool hasMtuOffsetOpenVpn = false;
int mtuOffsetOpenVpn = 0;
bool hasTunnelTestAttempts = false;
int tunnelTestAttempts = 0;
bool tunnelTestNoError = false;

void reset()
{
    remoteIp.clear();
    extraConfigForOpenVpn.clear();
    hasMtuOffsetOpenVpn = false;
    mtuOffsetOpenVpn = 0;
    hasTunnelTestAttempts = false;
    tunnelTestAttempts = 0;
    tunnelTestNoError = false;
    ExtraConfig::instance().setDetectedIp(QString());
}

} // namespace ExtraConfigMock

// Link-seam double for the class declared in utils/extraconfig.h: defines every method the
// production extraconfig.cpp defines, knob-driven where tests need control and inert elsewhere.
ExtraConfig::ExtraConfig() : fileWatcher_(nullptr)
{
}

void ExtraConfig::writeConfig(const QString &, bool) {}
QString ExtraConfig::getExtraConfig() { return QString(); }
std::optional<QString> ExtraConfig::getValue(const QString &) { return std::nullopt; }
QString ExtraConfig::getExtraConfigForOpenVpn() { return ExtraConfigMock::extraConfigForOpenVpn; }
QString ExtraConfig::getExtraConfigForIkev2() { return QString(); }
bool ExtraConfig::isUseIkev2Compression() { return false; }
QString ExtraConfig::getRemoteIpFromExtraConfig() { return ExtraConfigMock::remoteIp; }
int ExtraConfig::getMtuOffsetIkev2(bool &success) { success = false; return 0; }
int ExtraConfig::getMtuOffsetOpenVpn(bool &success) { success = ExtraConfigMock::hasMtuOffsetOpenVpn; return ExtraConfigMock::mtuOffsetOpenVpn; }
int ExtraConfig::getMtuOffsetWireguard(bool &success) { success = false; return 0; }
int ExtraConfig::getTunnelTestStartDelay(bool &success) { success = false; return 0; }
int ExtraConfig::getTunnelTestTimeout(bool &success) { success = false; return 0; }
int ExtraConfig::getTunnelTestRetryDelay(bool &success) { success = false; return 0; }
int ExtraConfig::getTunnelTestAttempts(bool &success) { success = ExtraConfigMock::hasTunnelTestAttempts; return ExtraConfigMock::tunnelTestAttempts; }
bool ExtraConfig::getIsTunnelTestNoError() { return ExtraConfigMock::tunnelTestNoError; }
bool ExtraConfig::getOverrideUpdateChannelToInternal() { return false; }
bool ExtraConfig::getIsStaging() { return false; }
bool ExtraConfig::getLogAPIResponse() { return false; }
bool ExtraConfig::getLogCtrld() { return false; }
bool ExtraConfig::getLogPings() { return false; }
bool ExtraConfig::getLogSplitTunnelExtension() { return false; }
bool ExtraConfig::getWireGuardVerboseLogging() { return false; }
bool ExtraConfig::getUsingScreenTransitionHotkeys() { return false; }
bool ExtraConfig::getUseICMPPings() { return false; }
bool ExtraConfig::getStealthExtraTLSPadding() { return false; }
bool ExtraConfig::getSuppressApiToken() { return false; }
bool ExtraConfig::getNoPings() { return false; }
std::optional<QString> ExtraConfig::serverlistCountryOverride() { return std::nullopt; }
bool ExtraConfig::serverListIgnoreCountryOverride() { return false; }
bool ExtraConfig::haveServerListCountryOverride() { return false; }
int ExtraConfig::getInt(const QString &, bool &success) { success = false; return 0; }
bool ExtraConfig::getFlag(const QString &) { return false; }
QString ExtraConfig::getString(const QString &) { return QString(); }
bool ExtraConfig::isLegalOpenVpnCommand(const QString &) const { return false; }
QString ExtraConfig::apiRootOverride() { return QString(); }
QString ExtraConfig::assetsRootOverride() { return QString(); }
bool ExtraConfig::useOpenVpnDCO() { return true; }
void ExtraConfig::fromJson(const QJsonObject &) {}
void ExtraConfig::logExtraConfig() {}
QJsonObject ExtraConfig::toJson() { return QJsonObject(); }
void ExtraConfig::parseConfigFile() {}
void ExtraConfig::onFileChanged() {}
