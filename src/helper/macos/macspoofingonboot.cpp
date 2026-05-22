#include "macspoofingonboot.h"
#include <filesystem>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include "../common/io_posix.h"
#include "../common/validation_posix.h"
#include "utils.h"

MacSpoofingOnBootManager::MacSpoofingOnBootManager()
{
}

MacSpoofingOnBootManager::~MacSpoofingOnBootManager()
{
}

bool MacSpoofingOnBootManager::setEnabled(bool bEnabled, const std::string &interface, const std::string &macAddress)
{
    if (bEnabled) {
        return enable(interface, macAddress);
    }
    return disable();
}

bool MacSpoofingOnBootManager::enable(const std::string &interface, const std::string &macAddress)
{
    // The boot script runs as root via launchd; both inputs are interpolated unescaped into shell
    // command lines, so reject anything outside the allowlist as defense in depth against the IPC
    // layer ever passing through unsanitized values.
    if (!Validation::isValidInterfaceName(interface)) {
        spdlog::error("MacSpoofingOnBootManager::enable: invalid interface name");
        return false;
    }
    if (!Validation::isValidMacAddress(macAddress)) {
        spdlog::error("MacSpoofingOnBootManager::enable: invalid MAC address");
        return false;
    }

    // write script
    std::stringstream script;

    script << "#!/bin/bash\n";
    script << "FILE=\"" WS_MAC_APP_DIR "/Contents/MacOS/" WS_APP_IDENTIFIER "\"\n";
    script << "if [ ! -f \"$FILE\" ]\n";
    script << "then\n";
    script << "echo \"File $FILE does not exists\"\n";
    script << "launchctl stop com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on\n";
    script << "launchctl unload /Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist\n";
    script << "launchctl remove com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on\n";
    script << "srm \"$0\"\n";
    script << "else\n";
    script << "echo \"File $FILE exists\"\n";
    script << "ipconfig waitall\n";
    script << "sleep 3\n";
    script << "/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport -z\n";
    script << "/sbin/ifconfig " << interface << " ether " << macAddress << "\n";
    script << "/sbin/ifconfig " << interface << " up\n";
    script << "fi\n";

    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    unlink(WS_POSIX_CONFIG_DIR "/boot_macspoofing.sh");
    int fd = open(WS_POSIX_CONFIG_DIR "/boot_macspoofing.sh", O_CREAT | O_WRONLY | O_TRUNC, 0700);
    if (fd < 0) {
        spdlog::error("Could not open boot script for writing");
        return false;
    }

    if (!IO::writeAll(fd, script.str())) {
        spdlog::error("Failed to write boot_macspoofing.sh: {}", IO::strerror(errno));
        close(fd);
        unlink(WS_POSIX_CONFIG_DIR "/boot_macspoofing.sh");
        return false;
    }
    close(fd);

    // write plist
    std::stringstream plist;

    plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    plist << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    plist << "<plist version=\"1.0\">\n";
    plist << "<dict>\n";
    plist << "<key>Label</key>\n";
    plist << "<string>com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on</string>\n";

    plist << "<key>ProgramArguments</key>\n";
    plist << "<array>\n";
    plist << "<string>/bin/bash</string>\n";
    plist << "<string>" WS_POSIX_CONFIG_DIR "/boot_macspoofing.sh</string>\n";
    plist << "</array>\n";

    plist << "<key>StandardErrorPath</key>\n";
    plist << "<string>/var/log/" WS_PRODUCT_NAME_LOWER "_macspoofing.log</string>\n";
    plist << "<key>StandardOutPath</key>\n";
    plist << "<string>/var/log/" WS_PRODUCT_NAME_LOWER "_macspoofing.log</string>\n";

    plist << "<key>RunAtLoad</key>\n";
    plist << "<true/>\n";
    plist << "</dict>\n";
    plist << "</plist>\n";

    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    unlink("/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist");
    fd = open("/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        spdlog::error("Could not open boot plist for writing");
        return false;
    }
    if (!IO::writeAll(fd, plist.str())) {
        spdlog::error("Failed to write macspoofing_on.plist: {}", IO::strerror(errno));
        close(fd);
        unlink("/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist");
        return false;
    }
    close(fd);
    Utils::executeCommand("launchctl", {"load", "-w", "/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist"});

    return true;
}

bool MacSpoofingOnBootManager::disable()
{
    Utils::executeCommand("launchctl", {"unload", "/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist"});
    std::error_code ec;
    std::filesystem::remove("/Library/LaunchDaemons/com.aaa." WS_PRODUCT_NAME_LOWER ".macspoofing_on.plist", ec);
    if (ec) {
        spdlog::warn("Failed to remove macspoofing plist: {}", ec.message());
    }
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/boot_macspoofing.sh", ec);
    if (ec) {
        spdlog::warn("Failed to remove boot_macspoofing.sh: {}", ec.message());
    }
    return true;
}
