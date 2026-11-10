#include "help.h"

#include <QList>
#include <QStringList>

#include "version/ws_version.h"

namespace {

struct CommandHelp
{
    QString name;
    QString group;
    QString summary;
    QStringList usage;
    // Free-form sections (arguments, flags, notes, examples) shown below the usage lines.
    QString details;
};

#ifdef Q_OS_LINUX
const QString kProtocols = "wireguard, udp, tcp, stealth, wstunnel";
#else
const QString kProtocols = "wireguard, ikev2, udp, tcp, stealth, wstunnel";
#endif

const QStringList kGroups = { "Connection", "Locations", "IP address", "Account", "Application" };

const QList<CommandHelp> &commands()
{
    static const QList<CommandHelp> list = {
        {
            "connect", "Connection",
            "Connect to a location (last used, or best if none)",
            {
                "connect [-n] [protocol[:port]]",
                "connect [-n] best [protocol[:port]]",
                "connect [-n] <location> [protocol[:port]]",
                "connect [-n] static <city|ip> [protocol[:port]]",
            },
            "Waits until the connection succeeds or fails, unless -n is given.\n"
            "\n"
            "Arguments:\n"
            "  best         The best location for your network\n"
            "  location     Country code, region, city, or nickname (case-insensitive).\n"
            "               Connects to a random datacenter in it.\n"
            "  static       One of your static IPs, by city or IP, exactly as\n"
            "               \"windscribe-cli locations static\" prints it\n"
            "  protocol     " + kProtocols + "\n"
            "  port         One of \"windscribe-cli ports <protocol>\". Defaults to the first.\n"
            "\n"
            "Flags:\n"
            "  -n           Return as soon as the request is sent\n"
            "\n"
            "Examples:\n"
            "  windscribe-cli connect\n"
            "  windscribe-cli connect CA\n"
            "  windscribe-cli connect Toronto wireguard\n"
            "  windscribe-cli connect -n best stealth:443\n"
        },
        {
            "disconnect", "Connection",
            "Disconnect from the VPN",
            { "disconnect [-n]" },
            "Waits until disconnected, unless -n is given.\n"
            "\n"
            "Flags:\n"
            "  -n           Return as soon as the request is sent\n"
        },
        {
            "status", "Connection",
            "Show connection, firewall, and account status",
            { "status" },
            "Prints one \"Label: value\" line per item, in this order:\n"
            "\n"
            "  Login        Logged in, Logged out, Logging in, or Error: <reason>\n"
            "  Connection   Connected to <city>, Connecting to <city>, Disconnecting,\n"
            "               Disconnected, or Error: <reason>. Shown when not logged out.\n"
            "               \"(checking the connection)\" or \"(network interference\n"
            "               detected)\" can follow \"Connected to <city>\".\n"
            "  Protocol     <protocol>:<port>, when connected\n"
            "  IP           <ip> (VPN) when connected, <ip> (public) when disconnected.\n"
            "               Hidden until the IP has been looked up after a connect or\n"
            "               disconnect, and while connected if the connection check\n"
            "               failed.\n"
            "  Firewall     On, Off, or Always On\n"
            "  Data usage   <used> / <limit or Unlimited>, when logged in\n"
            "  Internet     Available or Unavailable\n"
            "  Update       <version> available, only when there is one\n"
        },
        {
            "firewall", "Connection",
            "Turn the firewall on or off",
            { "firewall on|off" },
            "The firewall blocks all traffic outside the VPN. It cannot be turned off\n"
            "while it is set to Always on in the app's preferences.\n"
        },
        {
            "hydra", "Connection",
            "Show, import, or remove your hydra profile",
            { "hydra", "hydra import <file>", "hydra remove" },
            "A hydra profile, written by a hydra server you run, lists private entry\n"
            "points into Windscribe locations. While one is imported, connections to\n"
            "those locations use WireGuard through the entry point, and fail rather\n"
            "than connect without it.\n"
            "\n"
            "Commands:\n"
            "  (none)        Show the imported profile and the locations it fronts\n"
            "  import <file> Import a profile, replacing the one imported before\n"
            "  remove        Remove the profile; those locations connect directly again\n"
        },
        {
            "locations", "Locations",
            "List locations, favourites, or static IPs",
            { "locations [fav|static]" },
            "Prints one location per line:\n"
            "\n"
            "  locations          Region - City - Nickname\n"
            "  locations fav      City - Nickname [pinned IP]\n"
            "  locations static   City - IP\n"
            "\n"
            "A line ends with (Pro) if it needs Windscribe Pro, (Disabled) if it is\n"
            "temporarily unavailable, and (10 Gbps) on 10 Gbps servers.\n"
        },
        {
            "ports", "Locations",
            "List the ports available for a protocol",
            { "ports <protocol>" },
            "Arguments:\n"
            "  protocol     " + kProtocols + "\n"
            "\n"
            "Example:\n"
            "  windscribe-cli ports wireguard\n"
        },
        {
            "amneziawg", "Locations",
            "List the AmneziaWG censorship-circumvention presets",
            { "amneziawg" },
            "Prints one preset name per line. The list comes from Windscribe, so it is\n"
            "empty until the app has fetched it.\n"
            "\n"
#ifdef CLI_ONLY
            "AmneziaWG is used only when Protocol Tweaks is on. To use a preset, set\n"
            "ProtocolTweaksMethod=1 and AmneziawgPreset in the [Connection] section of\n"
            "~/.config/Windscribe/windscribe_cli.conf to the name exactly as printed,\n"
            "e.g. AmneziawgPreset=E - Baseline No Junk. A name that is not listed\n"
            "falls back to the first preset, or to a built-in one if the list was never\n"
            "fetched.\n"
            "\n"
            "ProtocolTweaksMethod values:\n"
            "  0  Auto: use a preset only when Windscribe suggests one (default)\n"
            "  1  Enabled: use AmneziawgPreset; if it is empty, the same fallback as above\n"
            "  2  Disabled: never use a preset\n"
            "\n"
            "CircumventCensorship does not turn AmneziaWG on.\n"
#else
            "AmneziaWG is used only when Protocol Tweaks is on. To use a preset, open\n"
            "Preferences > Connection > Anti-Censorship Settings in the app, set\n"
            "Protocol Tweaks to Enabled, and choose the preset under Amnezia Config.\n"
#endif
        },
        {
            "ip", "IP address",
            "Rotate, favourite, or unfavourite your VPN IP",
            { "ip rotate", "ip fav", "ip unfav <ip>" },
            "Commands:\n"
            "  rotate       Get a different IP on the current server\n"
            "  fav          Pin your current IP to your favourites\n"
            "  unfav <ip>   Remove a pinned IP from your favourites\n"
            "\n"
            "rotate and fav need a VPN connection, and Windscribe Pro or the current\n"
            "location in your plan. They exit with code 2 while a rotation is still\n"
            "in progress.\n"
        },
        {
            "login", "Account",
            "Log in to Windscribe",
            { "login [username [password]]" },
            "Prompts for the username and password if they are not given, and for a\n"
            "2FA code if your account needs one. Quote or escape a username or password\n"
            "that contains special characters.\n"
            "\n"
            "Prompts need a terminal. Without one (in a script, for example), the\n"
            "command never prompts: it reads the missing username, password, and 2FA\n"
            "code from stdin, one per line, and fails if stdin has none.\n"
            "\n"
            "Example:\n"
            "  printf '%s\\n' \"$PASSWORD\" | windscribe-cli login alice\n"
        },
        {
            "logout", "Account",
            "Log out, and turn the firewall off unless \"on\" is given",
            { "logout [on|off]" },
            "Disconnects from the VPN before logging out.\n"
            "\n"
            "Arguments:\n"
            "  on           Leave the firewall on\n"
            "  off          Turn the firewall off (default)\n"
        },
        {
            "update", "Application",
            "Download and install the available update",
            { "update" },
            "\"windscribe-cli status\" shows whether an update is available.\n"
        },
        {
            "logs", "Application",
            "Send debug logs to Windscribe support",
            { "logs send" },
            ""
        },
#ifdef CLI_ONLY
        {
            "keylimit", "Application",
            "Set what happens when the WireGuard key limit is reached",
            { "keylimit keep|delete" },
            "Applies to the next time the key limit is reached while the app runs.\n"
            "\n"
            "Arguments:\n"
            "  keep         Keep the oldest key, and fail the connection (default)\n"
            "  delete       Delete the oldest key, and connect\n"
        },
        {
            "ignoresslerrors", "Application",
            "Ignore SSL errors for this session, if login fails because of them",
            { "ignoresslerrors on|off" },
            "Turning this on disables TLS certificate validation, which could let a third\n"
            "party intercept your traffic. Only use it if logging in keeps failing with an\n"
            "SSL error.\n"
            "\n"
            "Arguments:\n"
            "  on           Ignore SSL errors and retry logging in\n"
            "  off          Stop ignoring SSL errors (default)\n"
        },
#endif
        {
            "version", "Application",
            "Print the version",
            { "version" },
            ""
        },
        {
            "help", "Application",
            "Show help for a command",
            { "help [command]" },
            ""
        },
    };
    return list;
}

const CommandHelp *find(const QString &command)
{
    for (const CommandHelp &help : commands()) {
        if (help.name == command) {
            return &help;
        }
    }
    return nullptr;
}

QString usageLines(const CommandHelp &help)
{
    QString out = "Usage:\n";
    for (const QString &line : help.usage) {
        out += "  windscribe-cli " + line + "\n";
    }
    return out;
}

} // namespace

namespace Help {

bool isCommand(const QString &command)
{
    return find(command) != nullptr;
}

QString overview()
{
    int width = 0;
    for (const CommandHelp &help : commands()) {
        width = qMax(width, static_cast<int>(help.name.length()));
    }

    QString out = QString("windscribe-cli %1: control the Windscribe VPN app from the command line\n").arg(WS_VERSION);
    out += "\n";
    out += "Usage:\n";
    out += "  windscribe-cli <command> [arguments]\n";

    for (const QString &group : kGroups) {
        out += "\n" + group + ":\n";
        for (const CommandHelp &help : commands()) {
            if (help.group == group) {
                out += "  " + help.name.leftJustified(width + 3) + help.summary + "\n";
            }
        }
    }

#ifdef CLI_ONLY
    out += "\n";
    out += "Preferences are in ~/.config/Windscribe/windscribe_cli.conf, and are applied\n";
    out += "as soon as the file is saved.\n";
#endif

    out += "\n";
    out += "Run \"windscribe-cli <command> --help\" for more information about a command.\n";
    return out;
}

QString command(const QString &command)
{
    const CommandHelp *help = find(command);
    if (help == nullptr) {
        return overview();
    }

    QString out = help->summary + "\n\n" + usageLines(*help);
    if (!help->details.isEmpty()) {
        out += "\n" + help->details;
    }
    return out;
}

QString usage(const QString &command)
{
    const CommandHelp *help = find(command);
    if (help == nullptr) {
        return QString();
    }
    return usageLines(*help);
}

} // namespace Help
