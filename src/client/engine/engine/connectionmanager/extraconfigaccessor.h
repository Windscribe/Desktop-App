#pragma once

#include "iextraconfigaccessor.h"

// Production IExtraConfigAccessor: sources every value from the ExtraConfig singleton.
class ExtraConfigAccessor : public IExtraConfigAccessor
{
public:
    int mtuOffsetOpenVpn(bool &hasValue) override;
    bool stealthExtraTLSPadding() override;
    QString remoteIp() override;
    QString detectedIp() override;
    bool useIkev2Compression() override;
    int tunnelTestAttempts(bool &hasValue) override;
    bool isTunnelTestNoError() override;
};
