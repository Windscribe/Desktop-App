#pragma once

// // Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#define WINVER 0x0601

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <Psapi.h>
#include <Rpc.h>
#include <Fwpmu.h>
#include <shlwapi.h>
#include <netcon.h>
#include <SetupAPI.h>
#include <strsafe.h>
#include <sddl.h>
#include <initguid.h>
#include <tlhelp32.h>
#include <aclapi.h>

// boost headers
#define BOOST_AUTO_LINK_TAGGED 1
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>


// C RunTime Header Files
#include <algorithm>
#include <assert.h>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <sstream>
#include <fstream>


