#pragma once

#include <QString>

// The connect-time tuning values ConnectionManager reads from ExtraConfig (the advanced/debug
// parameters file) while preparing a connection. This seam keeps CM off the ExtraConfig singleton
// directly so tests supply canned values instead of depending on what is on disk. Scoped to CM on
// purpose - it exposes the subset CM uses, not a general abstraction of ExtraConfig.
class IExtraConfigAccessor
{
public:
    virtual ~IExtraConfigAccessor() {}

    virtual bool logCtrld() = 0;
    virtual int mtuOffsetOpenVpn(bool &hasValue) = 0;
    virtual bool stealthExtraTLSPadding() = 0;
    virtual QString remoteIp() = 0;
    virtual QString detectedIp() = 0;
    virtual bool useIkev2Compression() = 0;
    virtual int tunnelTestAttempts(bool &hasValue) = 0;
    virtual bool isTunnelTestNoError() = 0;
};
