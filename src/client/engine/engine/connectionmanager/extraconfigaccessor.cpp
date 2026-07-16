#include "extraconfigaccessor.h"

#include "utils/extraconfig.h"

bool ExtraConfigAccessor::logCtrld()
{
    return ExtraConfig::instance().getLogCtrld();
}

int ExtraConfigAccessor::mtuOffsetOpenVpn(bool &hasValue)
{
    return ExtraConfig::instance().getMtuOffsetOpenVpn(hasValue);
}

bool ExtraConfigAccessor::stealthExtraTLSPadding()
{
    return ExtraConfig::instance().getStealthExtraTLSPadding();
}

QString ExtraConfigAccessor::remoteIp()
{
    return ExtraConfig::instance().getRemoteIpFromExtraConfig();
}

QString ExtraConfigAccessor::detectedIp()
{
    return ExtraConfig::instance().getDetectedIp();
}

bool ExtraConfigAccessor::useIkev2Compression()
{
    return ExtraConfig::instance().isUseIkev2Compression();
}

int ExtraConfigAccessor::tunnelTestAttempts(bool &hasValue)
{
    return ExtraConfig::instance().getTunnelTestAttempts(hasValue);
}

bool ExtraConfigAccessor::isTunnelTestNoError()
{
    return ExtraConfig::instance().getIsTunnelTestNoError();
}
