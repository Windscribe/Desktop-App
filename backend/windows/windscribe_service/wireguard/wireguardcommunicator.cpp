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

WireGuardCommunicator::Connection::Connection(const std::wstring &deviceName)
    : status_(Status::NO_ACCESS), pipeHandle_(INVALID_HANDLE_VALUE), fileHandle_(nullptr)
{
    PrivilegeHelper elevation;
    if (!elevation.checkElevation()) {
        Logger::instance().out(L"WireGuard Connection: Failed to elevate privileges");
        return;
    }
    std::wstring pipe_name(L"\\\\.\\pipe\\ProtectedPrefix\\Administrators\\WireGuard\\");
    pipe_name += deviceName;
    int attempt = 0;
    do {
        if (connect(pipe_name))
            break;
        if (++attempt < CONNECTION_ATTEMPT_COUNT)
            Sleep(CONNECTION_BETWEEN_WAIT_MS);
    } while (attempt < CONNECTION_ATTEMPT_COUNT);
    if (pipeHandle_ == INVALID_HANDLE_VALUE)
        return;
    if (!elevation.checkElevationForHandle(pipeHandle_)) {
        Logger::instance().out(L"WireGuard Connection: Pipe handle has invalid access rights");
        CloseHandle(pipeHandle_);
        pipeHandle_ = INVALID_HANDLE_VALUE;
        return;
    }
    const int fd = _open_osfhandle(reinterpret_cast<intptr_t>(pipeHandle_), _O_RDWR);
    if (fd == -1) {
        Logger::instance().out(L"WireGuard Connection: _open_osfhandle() failed");
        CloseHandle(pipeHandle_);
        pipeHandle_ = INVALID_HANDLE_VALUE;
        return;
    }
    fileHandle_ = _fdopen(fd, "r+");
    status_ = Status::OK;
}

WireGuardCommunicator::Connection::~Connection()
{
    if (fileHandle_)
        fclose(fileHandle_);
    if (pipeHandle_ != INVALID_HANDLE_VALUE)
        CloseHandle(pipeHandle_);
}

bool WireGuardCommunicator::Connection::getOutput(ResultMap *results_map) const
{
    if (!fileHandle_)
        return false;
    std::string output;
    output.reserve(1024);
    for (;;) {
        auto c = static_cast<char>(fgetc(fileHandle_));
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

bool WireGuardCommunicator::Connection::connect(const std::wstring &pipeName)
{
    pipeHandle_ = CreateFile(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, 0, nullptr);
    if (pipeHandle_ != INVALID_HANDLE_VALUE)
        return true;
    const auto error_code = GetLastError();
    switch (error_code) {
    case ERROR_FILE_NOT_FOUND:
        // Pipe is not available, don't attempt to reconnect.
        status_ = Status::NO_PIPE;
        return true;
    case ERROR_PIPE_BUSY:
        // Pipe is busy, may attempt to reconnect.
        status_ = Status::NO_PIPE;
        return false;
    default:
        // Other error, don't attempt to reconnect.
        status_ = Status::NO_ACCESS;
        return true;
    }
}

void WireGuardCommunicator::setDeviceName(const std::wstring &deviceName)
{
    assert(!deviceName.empty());
    deviceName_ = deviceName;
}

bool WireGuardCommunicator::configure(const std::string &clientPrivateKey,
    const std::string &peerPublicKey, const std::string &peerPresharedKey,
    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps)
{
    Connection connection(deviceName_);
    if (connection.getStatus() != Connection::Status::OK) {
        Logger::instance().out(L"WireGuardCommunicator::configure(): no connection to daemon");
        return false;
    }

    // Send set command.
    fputs("set=1\n", connection);
    fprintf(connection,
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
        fprintf(connection, "allowed_ip=%s\n",ip.c_str());
    fputs("\n", connection);
    fflush(connection);

    // Check results.
    Connection::ResultMap results{ std::make_pair("errno", "") };
    bool success = connection.getOutput(&results);
    if (success)
        success = stringToValue<int>(results["errno"]) == 0;
    return success;
}

UINT WireGuardCommunicator::getStatus(UINT32 *errorCode, UINT64 *bytesReceived,
                                      UINT64 *bytesTransmitted)
{
    Connection connection(deviceName_);
    const auto connection_status = connection.getStatus();
    if (connection_status != Connection::Status::OK) {
        if (connection.getStatus() == Connection::Status::NO_PIPE)
            return WIREGUARD_STATE_STARTING;
        if (errorCode)
            *errorCode = GetLastError();
        return WIREGUARD_STATE_ERROR;
    }

    // Send get command.
    fputs("get=1\n\n", connection);
    fflush(connection);

    Connection::ResultMap results{
        std::make_pair("errno", ""),
        std::make_pair("listen_port", ""),
        std::make_pair("public_key", ""),
        std::make_pair("rx_bytes", ""),
        std::make_pair("tx_bytes", ""),
        std::make_pair("last_handshake_time_sec", "")
    };
    bool success = connection.getOutput(&results);
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

bool WireGuardCommunicator::bindSockets(UINT if4, UINT if6, BOOL if6blackhole)
{
    // Check if nothing to bind.
    if (if4 == SOCKET_INTERFACE_KEEP && if6 == SOCKET_INTERFACE_KEEP)
        return true;

    Connection connection(deviceName_);
    if (connection.getStatus() != Connection::Status::OK) {
        Logger::instance().out(L"WireGuardCommunicator::bindSockets(): no connection to daemon");
        return false;
    }

    Logger::instance().out(L"WireGuardCommunicator::bindSockets(): %u %u(%u)",
        if4, if6, if6blackhole);

    // Send socket binding commands.
    fputs("set=1\n", connection);
    if (if4 != SOCKET_INTERFACE_KEEP)
        fprintf(connection,"bind_socket_if4=%u,0\n", if4);
    if (if6 != SOCKET_INTERFACE_KEEP)
        fprintf(connection, "bind_socket_if6=%u,%i\n", if6, if6blackhole);
    fputs("\n", connection);
    fflush(connection);

    // Check results.
    Connection::ResultMap results{ std::make_pair("errno", "") };
    bool success = connection.getOutput(&results);
    if (success)
        success = std::stoi(results["errno"]) == 0;
     return success;
}

void WireGuardCommunicator::quit()
{
    Connection connection(deviceName_);
    const auto connection_status = connection.getStatus();
    if (connection_status != Connection::Status::OK) {
        if (connection_status != Connection::Status::NO_PIPE)
            Logger::instance().out(L"WireGuardCommunicator::quit(): no connection to daemon");
      return;
    }
    // Send die command.
    fputs("die\n\n", connection);
    fflush(connection);
}
