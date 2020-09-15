#include "../all_headers.h"
#include "../ipc/servicecommunication.h"
#include "../logger.h"
#include "wireguardcommunicator.h"
#include "privilegehelper.h"
#include <boost/algorithm/string/trim.hpp>
#include <regex>
#include <type_traits>
#include <io.h>
#include <fcntl.h>

namespace
{
template<typename T>
T stringToValueImpl(const std::string &str)
{
    return static_cast<T>(strtol(str.c_str(), nullptr, 10));
}
template<>
UINT64 stringToValueImpl(const std::string &str)
{
    return _strtoui64(str.c_str(), nullptr, 10);
}
template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
stringToValue(const std::string &str)
{
    return str.empty() ? T(0) : stringToValueImpl<T>(str);
}
}  // namespace

void WireGuardCommunicator::setDeviceName(const std::wstring &deviceName)
{
    assert(!deviceName.empty());
    deviceName_ = deviceName;
}

bool WireGuardCommunicator::configure(const std::string &clientPrivateKey,
    const std::string &peerPublicKey, const std::string &peerPresharedKey,
    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps) const
{
    FILE *conn = nullptr;
    if (openConnection(&conn) != OpenConnectionState::OK) {
        Logger::instance().out(L"WireGuardCommunicator::configure(): no connection to daemon");
        return false;
    }

    // Send set command.
    fputs("set=1\n", conn);
    fprintf(conn,
        "private_key=%s\n"
        "replace_peers=true\n"
        "public_key=%s\n"
        "preshared_key=%s\n"
        "endpoint=%s\n"
        "persistent_keepalive_interval=0\n"
        "replace_allowed_ips=true\n",
        clientPrivateKey.c_str(), peerPublicKey.c_str(), peerPresharedKey.c_str(),
        peerEndpoint.c_str());
    for (const auto &ip : allowedIps)
        fprintf(conn, "allowed_ip=%s\n",ip.c_str());
    fputs("\n", conn);
    fflush(conn);

    // Check results.
    ResultMap results{ std::make_pair("errno", "") };
    bool success = getConnectionOutput(conn, &results);
    if (success)
        success = stringToValue<int>(results["errno"]) == 0;
    fclose(conn);
    return success;
}

UINT WireGuardCommunicator::getStatus(UINT32 *errorCode, UINT64 *bytesReceived,
                                      UINT64 *bytesTransmitted) const
{
    FILE *conn = nullptr;
    const auto result = openConnection(&conn);
    if (result == OpenConnectionState::NO_ACCESS) {
        if (errorCode)
            *errorCode = GetLastError();
        return WIREGUARD_STATE_ERROR;
    } else if (result == OpenConnectionState::NO_PIPE) {
        return WIREGUARD_STATE_STARTING;
    }

    // Send get command.
    fputs("get=1\n\n", conn);
    fflush(conn);

    ResultMap results{
        std::make_pair("errno", ""),
        std::make_pair("listen_port", ""),
        std::make_pair("public_key", ""),
        std::make_pair("rx_bytes", ""),
        std::make_pair("tx_bytes", ""),
        std::make_pair("last_handshake_time_sec", "")
    };
    bool success = getConnectionOutput(conn, &results);
    fclose(conn);
    if (!success)
        return WIREGUARD_STATE_STARTING;

    // Check for errors.
    const auto errno_value = stringToValue<UINT32>(results["errno"]);
    if (errno_value != 0) {
        if (errorCode)
            *errorCode = errno_value;
        return WIREGUARD_STATE_ERROR;
    }

    // Check if not yet listening.
    if (results["listen_port"].empty())
        return WIREGUARD_STATE_STARTING;

    // Check for handshake.
    if (stringToValue<UINT64>(results["last_handshake_time_sec"]) > 0) {
        if (bytesReceived)
            *bytesReceived = stringToValue<UINT64>(results["rx_bytes"]);
        if (bytesTransmitted)
            *bytesTransmitted = stringToValue<UINT64>(results["tx_bytes"]);
        return WIREGUARD_STATE_ACTIVE;
    }

    // If endpoint is set, we are connecting, otherwise simply listening.
    if (!results["public_key"].empty())
        return WIREGUARD_STATE_CONNECTING;
    return WIREGUARD_STATE_LISTENING;
}

bool WireGuardCommunicator::bindSockets(UINT if4, UINT if6, BOOL if6blackhole) const
{
    // Check if nothing to bind.
    if (if4 == SOCKET_INTERFACE_KEEP && if6 == SOCKET_INTERFACE_KEEP)
        return true;

    FILE *conn = nullptr;
    if (openConnection(&conn) != OpenConnectionState::OK) {
        Logger::instance().out(L"WireGuardCommunicator::bindSockets(): no connection to daemon");
        return false;
    }

    Logger::instance().out(L"WireGuardCommunicator::bindSockets(): %u %u(%u)",
        if4, if6, if6blackhole);

    // Send socket binding commands.
    fputs("set=1\n", conn);
    if (if4 != SOCKET_INTERFACE_KEEP)
        fprintf(conn,"bind_socket_if4=%u,0\n", if4);
    if (if6 != SOCKET_INTERFACE_KEEP)
        fprintf(conn, "bind_socket_if6=%u,%i\n", if6, if6blackhole);
    fputs("\n", conn);
    fflush(conn);

    // Check results.
    ResultMap results{ std::make_pair("errno", "") };
    bool success = getConnectionOutput(conn, &results);
    if (success)
        success = std::stoi(results["errno"]) == 0;
    fclose(conn);
    return success;
}

void WireGuardCommunicator::quit() const
{
    FILE *conn = nullptr;
    const auto result = openConnection(&conn);
    if (result != OpenConnectionState::OK) {
        if (result != OpenConnectionState::NO_PIPE)
            Logger::instance().out(L"WireGuardCommunicator::quit(): no connection to daemon");
      return;
    }
    // Send die command.
    fputs("die\n\n", conn);
    fflush(conn);
    fclose(conn);
}

WireGuardCommunicator::OpenConnectionState WireGuardCommunicator::openConnection(FILE **pfp) const
{
    assert(pfp != nullptr);
    PrivilegeHelper elevation;
    if (!elevation.checkElevation()) {
        Logger::instance().out(L"WireguardCommunicator: Failed to elevate privileges");
        return OpenConnectionState::NO_ACCESS;
    }
    std::wstring pipe_name(L"\\\\.\\pipe\\ProtectedPrefix\\Administrators\\WireGuard\\");
    pipe_name += deviceName_;
    HANDLE pipe_handle = CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                    OPEN_EXISTING, 0, nullptr);
    if (pipe_handle == INVALID_HANDLE_VALUE) {
        const auto error_code = GetLastError();
        if (error_code == ERROR_FILE_NOT_FOUND)
            return OpenConnectionState::NO_PIPE;
        else
            return OpenConnectionState::NO_ACCESS;
    }
    if (!elevation.checkElevationForHandle(pipe_handle)) {
        Logger::instance().out(L"WireguardCommunicator: Pipe handle has invalid access rights");
        CloseHandle(pipe_handle);
        return OpenConnectionState::NO_ACCESS;
    }
    const int fd = _open_osfhandle(reinterpret_cast<intptr_t>(pipe_handle), _O_RDWR);
    if (fd == -1) {
        Logger::instance().out(L"WireguardCommunicator: _open_osfhandle() failed");
        CloseHandle(pipe_handle);
        return OpenConnectionState::NO_ACCESS;
    }
    *pfp = _fdopen(fd, "r+");
    return OpenConnectionState::OK;
}

bool WireGuardCommunicator::getConnectionOutput(FILE *conn, ResultMap *results_map) const
{
    std::string output;
    output.reserve(1024);
    for (;;) {
        auto c = static_cast<char>(fgetc(conn));
        if (c < 0)
            break;
        output.push_back(c);
    }
    boost::trim(output);
    if (output.empty())
        return false;
    if (results_map && !results_map->empty()) {
        std::regex output_rx("(^|\n)(\\w+)=(\\w+)(?=\n|$)");
        std::sregex_iterator it(output.begin(), output.end(), output_rx), end;
        for (; it != end; ++it) {
            if (it->size() != 4)
                continue;
            const auto &match = *it;
            auto mapitem = results_map->find(match[2].str());
            if (mapitem != results_map->end())
                mapitem->second = match[3].str();
        }
    }
    return true;
}
