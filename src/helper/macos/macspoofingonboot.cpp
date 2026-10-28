#include "macspoofingonboot.h"

#include <filesystem>
#include <spdlog/spdlog.h>

#include "utils.h"

namespace MacSpoofingOnBoot {

void remove()
{
    // The common case is nothing armed; don't spawn launchctl against a plist that isn't there.
    std::error_code ec;
    const bool plistExists = std::filesystem::exists(
        "/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist", ec);
    if (ec) {
        spdlog::warn("Failed to check for macspoofing plist: {}", ec.message());
    }
    if (plistExists || ec) {
        Utils::executeCommand("launchctl", {"unload", "/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist"});
    }
    ec.clear();
    std::filesystem::remove("/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist", ec);
    if (ec) {
        spdlog::warn("Failed to remove macspoofing plist: {}", ec.message());
    }
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/boot_macspoofing.sh", ec);
    if (ec) {
        spdlog::warn("Failed to remove boot_macspoofing.sh: {}", ec.message());
    }
}

} // namespace MacSpoofingOnBoot
