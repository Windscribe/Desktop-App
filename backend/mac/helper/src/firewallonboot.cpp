#include "firewallonboot.h"
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "logger.h"
#include "utils.h"

FirewallOnBootManager::FirewallOnBootManager()
{
}

FirewallOnBootManager::~FirewallOnBootManager()
{
}

bool FirewallOnBootManager::setEnabled(bool bEnabled, const std::string &rules)
{
    if (bEnabled) {
        return enable(rules);
    }
    return disable();
}

bool FirewallOnBootManager::enable(const std::string &rules) {
    // write rules
    int fd = open("/etc/windscribe/boot_pf.conf", O_CREAT | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        LOG("Could not open boot firewall rules for writing");
    	return false;
    }

    write(fd, rules.c_str(), rules.length());
    close(fd);

    // write script
    std::stringstream script;

    script << "#!/bin/bash\n";
    script << "FILE=\"/Applications/Windscribe.app/Contents/MacOS/Windscribe\"\n";
    script << "if [ ! -f \"$FILE\" ]\n";
    script << "then\n";
    script << "echo \"File $FILE does not exists\"\n";
    script << "launchctl stop com.aaa.windscribe.firewall_on\n";
    script << "launchctl unload /Library/LaunchDaemons/com.aaa.windscribe.firewall_on.plist\n";
    script << "launchctl remove com.aaa.windscribe.firewall_on\n";
    script << "srm \"$0\"\n";
    script << "else\n";
    script << "echo \"File $FILE exists\"\n";
    script << "ipconfig waitall\n";
    script << "/sbin/pfctl -e -f \"/etc/windscribe/boot_pf.conf\"\n";
    script << "fi\n";

    fd = open("/etc/windscribe/boot_pf.sh", O_CREAT | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        LOG("Could not open boot script for writing");
    	return false;
    }

    write(fd, script.str().c_str(), script.str().length());
    close(fd);
    Utils::executeCommand("chmod", {"+x", "/etc/windscribe/boot_pf.sh"});

    // write plist
    std::stringstream plist;

    plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    plist << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    plist << "<plist version=\"1.0\">\n";
    plist << "<dict>\n";
    plist << "<key>Label</key>\n";
    plist << "<string>com.aaa.windscribe.firewall_on</string>\n";

    plist << "<key>ProgramArguments</key>\n";
    plist << "<array>\n";
    plist << "<string>/bin/bash</string>\n";
    plist << "<string>/etc/windscribe/boot_pf.sh</string>\n";
    plist << "</array>\n";

    plist << "<key>StandardErrorPath</key>\n";
    plist << "<string>/var/log/windscribe_pf.log</string>\n";
    plist << "<key>StandardOutPath</key>\n";
    plist << "<string>/var/log/windscribe_pf.log</string>\n";

    plist << "<key>RunAtLoad</key>\n";
    plist << "<true/>\n";
    plist << "</dict>\n";
    plist << "</plist>\n";

    fd = open("/Library/LaunchDaemons/com.aaa.windscribe.firewall_on.plist", O_CREAT | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        LOG("Could not open boot plist for writing");
    	return false;
    }
    write(fd, plist.str().c_str(), plist.str().length());
    close(fd);
    Utils::executeCommand("launchctl", {"load", "-w", "/Library/LaunchDaemons/com.aaa.windscribe.firewall_on.plist"});

    return true;
}

bool FirewallOnBootManager::disable()
{
    Utils::executeCommand("launchctl", {"unload", "/Library/LaunchDaemons/com.aaa.windscribe.firewall_on.plist"});
    Utils::executeCommand("rm", {"-f", "/Library/LaunchDaemons/com.aaa.windscribe.firewall_on.plist"});
    Utils::executeCommand("rm", {"-f", "/etc/windscribe/boot_pf.conf"});
    Utils::executeCommand("rm", {"-f", "/etc/windscribe/boot_pf.sh"});
    return true;
}