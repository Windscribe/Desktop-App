#pragma once

#include <QString>

// Knobs read by the link-substituted ExtraConfig implementation (extraconfig_mock.cpp). A test
// target that compiles the mock gets it instead of the production extraconfig.cpp: the mock object
// already defines every ExtraConfig symbol, so the archive member is never pulled in.
namespace ExtraConfigMock {

extern QString remoteIp;
extern QString extraConfigForOpenVpn;
extern bool hasMtuOffsetOpenVpn;
extern int mtuOffsetOpenVpn;
extern bool hasTunnelTestAttempts;
extern int tunnelTestAttempts;
extern bool tunnelTestNoError;

// Restores all knobs (and the instance's detected ip) to defaults; call from the fixture's init().
void reset();

} // namespace ExtraConfigMock
