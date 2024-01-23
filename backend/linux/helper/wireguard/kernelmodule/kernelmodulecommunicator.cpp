#include "../../../../posix_common/helper_commands.h"
#include "kernelmodulecommunicator.h"
#include "../../logger.h"
#include "../../utils.h"
#include "wireguard.h"
#include <boost/asio.hpp>
#include <boost/algorithm/hex.hpp>
#include <sys/socket.h>

bool KernelModuleCommunicator::start(
    const std::string &exePath,
    const std::string &executable,
    const std::string &deviceName)
{
    UNUSED(exePath);
    UNUSED(executable);
    assert(!deviceName.empty());
    // NULL terminator included in size of IFNAMSIZ so device name must be smaller
    if (deviceName.size() >= IFNAMSIZ) {
        return false;
    }
    deviceName_ = deviceName;
    return true;
}

bool KernelModuleCommunicator::stop()
{
    wg_del_device(deviceName_.c_str());
    return true;
}

bool KernelModuleCommunicator::setPeerAllowedIps(wg_peer *peer, const std::vector<std::string> &ips)
{
    peer->first_allowedip = (wg_allowedip *)calloc(ips.size(), sizeof(wg_allowedip));
    if (peer->first_allowedip == nullptr)
        return false;

    wg_allowedip *cur = peer->first_allowedip;
    boost::system::error_code ec;

    for (size_t i = 0; i < ips.size(); i++)
    {
        size_t cidrsep = ips[i].rfind('/');
        if (cidrsep == std::string::npos)
        {
            freeAllowedIps(peer->first_allowedip);
            peer->first_allowedip = nullptr;
            return false;
        }
        errno = 0;
        long cidr = strtol(ips[i].substr(cidrsep+1).c_str(), nullptr, 10);
        if (errno != 0)
        {
            freeAllowedIps(peer->first_allowedip);
            peer->first_allowedip = nullptr;
            return false;
        }
        cur->cidr = cidr;

        boost::asio::ip::address addr = boost::asio::ip::make_address(ips[i].substr(0, cidrsep).c_str(), ec);
        if (ec)
        {
            freeAllowedIps(peer->first_allowedip);
            peer->first_allowedip = nullptr;
            return false;
        }

        if (addr.is_v4())
        {
            cur->family = AF_INET;
            memcpy(&cur->ip4, addr.to_v4().to_bytes().data(), 4);
        }
        else
        {
            cur->family = AF_INET6;
            memcpy(&cur->ip6, addr.to_v6().to_bytes().data(), 16);
        }

        if (i + 1 < ips.size())
           cur->next_allowedip = &peer->first_allowedip[i+1];
        cur = cur->next_allowedip;
    }
    peer->last_allowedip = cur;
    return true;
}

void KernelModuleCommunicator::freeAllowedIps(wg_allowedip *ips)
{
    if (ips == nullptr)
        return;
    free(ips);
}

bool KernelModuleCommunicator::setPeerEndpoint(wg_peer *peer, const std::string &endpoint)
{
    // Technically, this is incorrect, because for IPv6, we should have an endpoint such as
    // [addr]:port, because colons have overloaded usage and may be ambiguous.  However,
    // the engine currently blindly constructs addr:port, so assume the number after the
    // last colon represents the port.
    size_t portsep = endpoint.rfind(':');
    long port = 0;
    boost::system::error_code ec;

    if (portsep == std::string::npos)
        return false;

    errno = 0;
    port = strtol(endpoint.substr(portsep+1).c_str(), nullptr, 10);
    if (port == 0 || errno != 0)
        return false;

    boost::asio::ip::address addr = boost::asio::ip::make_address(endpoint.substr(0, portsep), ec);
    if (ec)
        return false;

    if (addr.is_v4())
    {
        peer->endpoint.addr4.sin_family = AF_INET;
        peer->endpoint.addr4.sin_port = htons(port);
        memcpy(&peer->endpoint.addr4.sin_addr.s_addr, addr.to_v4().to_bytes().data(), 4);
    }
    else
    {
        peer->endpoint.addr6.sin6_family = AF_INET6;
        peer->endpoint.addr6.sin6_port = htons(port);
        memcpy(&peer->endpoint.addr6.sin6_addr.s6_addr, addr.to_v6().to_bytes().data(), 16);
    }

    return true;
}

bool KernelModuleCommunicator::configure(const std::string &clientPrivateKey,
    const std::string &peerPublicKey, const std::string &peerPresharedKey,
    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps,
    uint32_t fwmark, uint16_t listenPort)
{
    std::string buf;
    wg_peer new_peer;

    memset(&new_peer, 0, sizeof(wg_peer));

    new_peer.flags = (enum wg_peer_flags)0;
    if (!peerPublicKey.empty())
    {
        new_peer.flags = (enum wg_peer_flags)(WGPEER_HAS_PUBLIC_KEY | new_peer.flags);
        buf = boost::algorithm::unhex(peerPublicKey);
        if (buf.size() != sizeof(wg_key))
            return false;
        memcpy(&new_peer.public_key, buf.c_str(), sizeof(wg_key));
    }
    if (!peerPresharedKey.empty())
    {
        new_peer.flags = (enum wg_peer_flags)(WGPEER_HAS_PRESHARED_KEY | new_peer.flags);
        buf = boost::algorithm::unhex(peerPresharedKey);
        if (buf.size() != sizeof(wg_key))
            return false;
        memcpy(&new_peer.preshared_key, buf.c_str(), sizeof(wg_key));
    }

    if (!setPeerEndpoint(&new_peer, peerEndpoint))
        return false;

    if (allowedIps.size() != 0)
    {
        new_peer.flags = (enum wg_peer_flags)(WGPEER_REPLACE_ALLOWEDIPS | new_peer.flags);
        if (!setPeerAllowedIps(&new_peer, allowedIps))
            return false;
    }

    wg_device new_device;
    memset(&new_device, 0, sizeof(wg_device));

    strcpy(new_device.name, deviceName_.c_str());
    new_device.flags = (enum wg_device_flags)(WGDEVICE_HAS_FWMARK);
    if (!clientPrivateKey.empty())
    {
        new_device.flags = (enum wg_device_flags)(WGDEVICE_HAS_PRIVATE_KEY | new_device.flags);
        buf = boost::algorithm::unhex(clientPrivateKey);
        if (buf.size() != sizeof(wg_key))
        {
            freeAllowedIps(new_peer.first_allowedip);
            return false;
        }
        memcpy(&new_device.private_key, buf.c_str(), sizeof(wg_key));
    }
    new_device.first_peer = &new_peer;
    new_device.last_peer = &new_peer;
    new_device.fwmark = fwmark;
    if (listenPort) {
        new_device.flags = (enum wg_device_flags)(WGDEVICE_HAS_LISTEN_PORT | new_device.flags);
        new_device.listen_port = listenPort;
    }

    if (wg_add_device(new_device.name) < 0 || wg_set_device(&new_device) < 0)
    {
        freeAllowedIps(new_peer.first_allowedip);
        wg_del_device(new_device.name);
        return false;
    }

    freeAllowedIps(new_peer.first_allowedip);
    return true;
}

unsigned long KernelModuleCommunicator::getStatus(unsigned int *errorCode,
    unsigned long long *bytesReceived, unsigned long long *bytesTransmitted)
{
    UNUSED(errorCode);

    wg_device *device = nullptr;
    if (wg_get_device(&device, deviceName_.c_str()) < 0)
        return kWgStateListening;

    if (device->first_peer != nullptr && device->first_peer->last_handshake_time.tv_sec > 0)
    {
        struct timeval tv;
        int rc = gettimeofday(&tv, NULL);
        if (rc || tv.tv_sec - device->first_peer->last_handshake_time.tv_sec > 180)
        {
            Logger::instance().out("Time since last handshake time exceeded 3 minutes, disconnecting");
            return kWgStateError;
        }
        *bytesReceived = device->first_peer->rx_bytes;
        *bytesTransmitted = device->first_peer->tx_bytes;
        wg_free_device(device);
        return kWgStateActive;
    }

    wg_free_device(device);
    return kWgStateConnecting;
}
