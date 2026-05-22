#include "bound_route.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

BoundRoute::BoundRoute() : isBoundRouteAdded_(false)
{

}

BoundRoute::~BoundRoute()
{
    remove();
}

void BoundRoute::create(const types::IpAddress &ipAddress, const std::string &interfaceName)
{
    remove();   // remove previous if exists

    if (!ipAddress.isValid()) {
        spdlog::error("BoundRoute::create(): invalid gateway IP, skipping");
        return;
    }

    // Refuse to install a v6 bound route via a link-local gateway (`fe80::/10`). The engine-side
    // NetworkUtils_mac::getDefaultRouteV6 strips the `%<iface>` scope suffix so types::IpAddress
    // can parse it, but `route add -inet6 -net :: fe80::1 -ifscope <iface>` cannot resolve a
    // bare link-local gateway — macOS `route` needs the scope baked into the gateway argument
    // (`fe80::1%<iface>`), and -ifscope only scopes the route entry itself, not the gateway
    // lookup. Common on residential ISPs that advertise the v6 default via fe80::1 RA. Skip
    // and let the kernel's existing default-route entry stand; the v4 boundRoute_ sibling
    // still pins v4 traffic to the chosen interface, and v6 falls through to the host's
    // existing v6 default route — same behaviour as before this branch landed v6 support.
    if (ipAddress.isV6()) {
        const uint8_t *b = ipAddress.bytes();
        const bool isLinkLocal = (b[0] == 0xFE) && ((b[1] & 0xC0) == 0x80);
        if (isLinkLocal) {
            spdlog::warn("BoundRoute::create(): refusing v6 bound route via link-local gateway {}, kernel default route will stand",
                         ipAddress.toString());
            return;
        }
    }

    ipAddress_ = ipAddress;
    interfaceName_ = interfaceName;

    const std::string gw = ipAddress_.toString();
    if (ipAddress_.isV6()) {
        // Dormant in practice: Windscribe IKEv2/OpenVPN/Stunnel/Wstunnel don't fill gatewayIpV6;
        // WG's v6 default-route bypass goes through `WireGuardAdapter::enableRouting`, not here.
        // Kept for symmetry so a future v6-capable OpenVPN config doesn't silently no-op.
        // `-net ::/0` is the canonical default-route form; matches `WireGuardAdapter::enableRouting`
        // and is easier to grep for than the equivalent `-net ::` shorthand.
        spdlog::info("execute: route add -inet6 -net ::/0 {} -ifscope {}", gw, interfaceName_);
        Utils::executeCommand("route", {"add", "-inet6", "-net", "::/0", gw, "-ifscope", interfaceName_});
    } else {
        spdlog::info("execute: route add -net 0.0.0.0/0 {} -ifscope {}", gw, interfaceName_);
        Utils::executeCommand("route", {"add", "-net", "0.0.0.0/0", gw, "-ifscope", interfaceName_});
    }
    isBoundRouteAdded_ = true;
}

void BoundRoute::remove()
{
    if (isBoundRouteAdded_)
    {
        const std::string gw = ipAddress_.toString();
        if (ipAddress_.isV6()) {
            spdlog::info("execute: route delete -inet6 -net ::/0 {} -ifscope {}", gw, interfaceName_);
            Utils::executeCommand("route", {"delete", "-inet6", "-net", "::/0", gw, "-ifscope", interfaceName_});
        } else {
            spdlog::info("execute: route delete -net 0.0.0.0/0 {} -ifscope {}", gw, interfaceName_);
            Utils::executeCommand("route", {"delete", "-net", "0.0.0.0/0", gw, "-ifscope", interfaceName_});
        }
        isBoundRouteAdded_ = false;
    }
}
