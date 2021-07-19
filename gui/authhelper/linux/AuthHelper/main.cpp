// AuthHelper
// This empty process is used as a helper process to check for root access dynamically

#include <string>
#include <fstream>
#include <iostream>

int main()
{
    return 0;
}

// temporarily saving here until this code moves to the installer, call like so:
// writeAuthHelperPolicyFile("/usr/share/polkit-1/actions/com.windscribe.authhelper.policy", "/ws/install/path/");
void writeAuthHelperPolicyFile(const std::string &filename, const std::string &authHelperInstallDir)
{
    std::fstream file;
    file.open(filename, std::ios_base::out);
    if (!file.is_open())
    {
        std::cout << "Failed to open policy file for writing" << std::endl;
        return;
    }

    file <<  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE policyconfig PUBLIC\n"
            " \"-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN\"\n"
            " \"http://www.freedesktop.org/software/polkit/policyconfig-1.dtd\">\n"
            "<policyconfig>\n"
            "  <vendor>Windscribe</vendor>\n"
            "  <action id=\"com.windscribe.authhelper.authenticate\">\n"
            "   <description>Run AuthHelper</description>\n"
            "   <message>Authentication is required to change to a non-root custom config directory.</message>\n"
            "    <defaults>\n"
            "      <allow_any>auth_admin_keep</allow_any>\n"
            "      <allow_inactive>auth_admin_keep</allow_inactive>\n"
            "      <allow_active>auth_admin_keep</allow_active>\n"
            "    </defaults>\n"
            "    <annotate key=\"org.freedesktop.policykit.exec.path\">" <<
            authHelperInstallDir << "windscribe-authhelper</annotate>\n"
            "    <annotate key=\"org.freedesktop.policykit.exec.allow_gui\">true</annotate>\n"
            "  </action>\n"
            "</policyconfig>\n";
    file.close();
}
