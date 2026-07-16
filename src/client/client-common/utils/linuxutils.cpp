#include "linuxutils.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "log/categories.h"

#include "utils.h"

namespace LinuxUtils
{

bool dropHelperGroup()
{
    // Raw per-thread setresgid, not glibc's wrapper: glibc broadcasts the change to every thread (the
    // setxid mechanism), which would strip the group from the connect thread (HelperConnector) before
    // it finishes connecting. Dropping the saved gid too makes the drop permanent.
    if (syscall(SYS_setresgid, getgid(), getgid(), getgid()) != 0) {
        qCCritical(LOG_BASIC) << "Could not drop group";
        return false;
    }
    return true;
}

QString getOsVersionString()
{
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        return QString(unameData.sysname) + " " + QString(unameData.version);
    }

    return "Can't detect OS Linux version";
}

QString getLinuxKernelVersion()
{
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        QRegularExpression rx("(\\d+\\.\\d+(\\.\\d+)*)");
        QRegularExpressionMatch match = rx.match(unameData.release);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }

    return QString("Can't detect Linux Kernel version");
}

const QString getLastInstallPlatform()
{
    static QString linuxPlatformName;
    static bool tried = false;

    // only read in file once, cache the result
    if (tried) return linuxPlatformName;
    tried = true;

    if (!QFile::exists(LAST_INSTALL_PLATFORM_FILE))
    {
        qCInfo(LOG_BASIC) << "Couldn't find previous install platform file: " << LAST_INSTALL_PLATFORM_FILE;
        return "";
    }

    QFile lastInstallPlatform(LAST_INSTALL_PLATFORM_FILE);

    if (!lastInstallPlatform.open(QIODevice::ReadOnly))
    {
        qCInfo(LOG_BASIC) << "Couldn't open previous install platform file: " << LAST_INSTALL_PLATFORM_FILE;
        return "";
    }

    linuxPlatformName = lastInstallPlatform.readAll().trimmed(); // remove newline
    return linuxPlatformName;
}

// Returns the exit code of a systemctl query verb, or -1 on timeout.
static int systemctlCheck(const char *verb, const QString &unit)
{
    QProcess process;
    process.start("systemctl", {"--quiet", QString(verb), unit});
    if (!process.waitForFinished(2000)) {
        process.kill();
        process.waitForFinished(500);
        return -1;
    }
    return process.exitCode();
}

bool isNetworkManagerActive()
{
    // A live "is it running" check is intentionally all we need here -- do NOT be tempted to also accept
    // "enabled but inactive" to guard against NetworkManager still coming up when we're consulted. There is
    // no such race: NetworkManager is a boot-time service that reaches active during early boot (network
    // targets, before the display manager and graphical session). Any Windscribe process -- including the
    // GUI autostarted at login and the engine it spawns -- starts long after that, so if NM is enabled it is
    // already active by the time we ask. The only ways this returns false are the genuine ones: NM is
    // stopped, disabled, masked, or not installed -- in every one of those cases the nmcli-backed paths
    // (DNS read, network-name lookup, MAC spoofing) genuinely can't work and should fall back or skip.
    //
    // systemctl answers from PID 1 and should never block; if it somehow does, err toward active so we
    // don't disable NetworkManager-backed features over a transient hiccup.
    const int ret = systemctlCheck("is-active", "NetworkManager.service");
    return ret == 0 || ret == -1;
}

bool isAppAlreadyRunning()
{
    // Look for process containing the product name -- exclude grep and Engine
    QString cmd = "ps axco command | grep " WS_APP_EXECUTABLE_NAME " | grep -v grep";
#ifdef WS_IS_WINDSCRIBE
    cmd += " | grep -v " WS_APP_IDENTIFIER "Engine"; // Exclude older 1.x engine process
#endif
    if (strlen(WS_CLI_EXECUTABLE_NAME) > 0)
        cmd += " | grep -v " WS_CLI_EXECUTABLE_NAME;
    QString response = Utils::execCmd(cmd);
    return response.trimmed() != "";
}

QMap<QString, QString> enumerateInstalledPrograms()
{
    QMap<QString, QString> programs;

    // On Linux, we are looking for desktop entries. These are located under the applications dir at ~/.local/share, and each dir in $XDG_DATA_DIRS
    QByteArray xdgDataDirs = qgetenv("XDG_DATA_DIRS");
    if (xdgDataDirs.isEmpty()) {
        xdgDataDirs = "/usr/local/share/:/usr/share/";
    }
    QStringList dirs = QString::fromLocal8Bit(xdgDataDirs).split(":");
    QByteArray xdgDataHome = qgetenv("XDG_DATA_HOME");
    if (xdgDataHome.isEmpty()) {
        dirs.prepend(QDir::homePath() + "/.local/share");
    } else {
        dirs.prepend(QString::fromLocal8Bit(xdgDataHome));
    }

    for (auto dir : dirs) {
        for (auto filename : QDir(dir + "/applications").entryList(QStringList("*.desktop"), QDir::Files)) {
            QString file = dir + "/applications/" + filename;
            // If we already have this application from a previous directory, ignore it
            if (programs.keys().contains(file)) {
                continue;
            }

            QFile f(file);
            if (!f.open(QFile::ReadOnly | QFile::Text)) {
                continue;
            }
            QTextStream in(&f);
            QStringList contents = in.readAll().split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);

            // Omit entries without Type=Application
            if (contents.indexOf(QRegularExpression("^Type=Application$")) == -1) {
                continue;
            }
            // Omit entries with NoDisplay=true
            if (contents.indexOf(QRegularExpression("^NoDisplay=true$")) != -1) {
                continue;
            }
            int idx = contents.indexOf(QRegularExpression("^Exec=.*"));
            if (idx == -1) {
                continue;
            }

            // Extract the contents of the first Exec= line
            QString exec = extractExeName(contents[idx].mid(5));
            if (!exec.isEmpty()) {
                int iconIdx = contents.indexOf(QRegularExpression("^Icon=.*"));
                if (iconIdx == -1) {
                    // best guess
                    programs[exec] = exec;
                } else {
                    programs[exec] = contents[iconIdx].mid(5);
                }
            }
        }
    }
    return programs;
}

QString extractExeName(const QString &execLine)
{
    QString remaining = execLine;
    QString block;
    QString path;
    bool isFlatpak = false;
    bool isSnap = false;

    // Special handling for snaps
    int idx = execLine.indexOf("/snap/bin/");
    if (idx != -1) {
        isSnap = true;
    }

    while (!remaining.isEmpty()) {
        int end;
        // if quoted, this block ends at the end quote, otherwise at the first space
        if (remaining[0] == '"') {
            end = remaining.indexOf('"', 1);
            block = unescape(remaining.mid(1, end - 1));
            remaining = remaining.mid(end + 1);
        } else {
            end = remaining.indexOf(' ');
            if (end == -1) {
                block = remaining;
                remaining = "";
            } else {
                block = remaining.first(end);
                remaining = remaining.mid(end + 1);
            }
        }

        if (isSnap && block.startsWith("/snap/bin/")) {
            return block;
        }

        // ignore env-var assignments and 'env'; this also filters --branch=, --command=, etc.
        if (block.contains("=") || block == "env") {
            continue;
        }

        // skip Exec field codes (%U, %F, ...) and Flatpak file-forwarding markers (@@, @@u, ...)
        if (block.startsWith("%") || block.startsWith("@@")) {
            continue;
        }

        if (isFlatpak) {
            // After 'flatpak run', the first reverse-DNS-shaped positional is the app ID
            // (e.g. org.mozilla.firefox). 'run' itself has no dot, so it's skipped here.
            if (block.contains(".")) {
                return block;
            }
            continue;
        }

        path = convertToAbsolutePath(block);
        if (!path.isEmpty()) {
            if (QFileInfo(QFile(path)).fileName() == "flatpak") {
                isFlatpak = true;
                continue;
            }
            if (isSnap) {
                continue;
            }

            return path;
        }
    }

    // If we got here, we didn't find an exe
    return QString();
}

// Walks all .desktop files reachable via XDG and, for every Flatpak entry, builds a mapping from
// keys a legacy stored entry might have (the basename of --command=, the .desktop filename stem)
// to the Flatpak app ID. Used by migrateSplitTunnelingFlatpakApps.
static QHash<QString, QString> enumerateFlatpakCommandMap()
{
    QHash<QString, QString> result;

    QByteArray xdgDataDirs = qgetenv("XDG_DATA_DIRS");
    if (xdgDataDirs.isEmpty()) {
        xdgDataDirs = "/usr/local/share/:/usr/share/";
    }
    QStringList dirs = QString::fromLocal8Bit(xdgDataDirs).split(":");
    QByteArray xdgDataHome = qgetenv("XDG_DATA_HOME");
    if (xdgDataHome.isEmpty()) {
        dirs.prepend(QDir::homePath() + "/.local/share");
    } else {
        dirs.prepend(QString::fromLocal8Bit(xdgDataHome));
    }

    for (auto dir : dirs) {
        for (auto filename : QDir(dir + "/applications").entryList(QStringList("*.desktop"), QDir::Files)) {
            QString file = dir + "/applications/" + filename;

            QFile f(file);
            if (!f.open(QFile::ReadOnly | QFile::Text)) {
                continue;
            }
            QTextStream in(&f);
            QStringList contents = in.readAll().split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);

            int execIdx = contents.indexOf(QRegularExpression("^Exec=.*"));
            if (execIdx == -1) {
                continue;
            }
            QString execLine = contents[execIdx].mid(5);

            QString appId = extractExeName(execLine);
            if (appId.isEmpty() || !appId.contains(".") || appId.contains("/")) {
                continue;  // not a Flatpak entry
            }

            // Map any --command=<value> basename to the app ID — this is what older clients stored.
            QRegularExpression cmdRe("--command=(\\S+)");
            auto m = cmdRe.match(execLine);
            if (m.hasMatch()) {
                QString cmdBasename = QFileInfo(m.captured(1)).fileName();
                if (!cmdBasename.isEmpty()) {
                    result.insert(cmdBasename, appId);
                }
            }

            // Also map the .desktop filename stem, in case a stored entry came from there.
            QString stem = filename;
            if (stem.endsWith(".desktop")) {
                stem.chop(QStringLiteral(".desktop").size());
            }
            if (!stem.isEmpty()) {
                result.insert(stem, appId);
            }
        }
    }

    return result;
}

void migrateSplitTunnelingFlatpakApps(QVector<types::SplitTunnelingApp> &apps)
{
    QHash<QString, QString> commandToId;
    bool scanned = false;

    for (auto &app : apps) {
        if (app.fullName.isEmpty()) continue;

        // Pick the key to look up in the basename-keyed map.
        //   "/app/bin/chrome"   -> "chrome"   (legacy Flatpak sandbox path from --command=/app/bin/chrome)
        //   "chrome"            -> "chrome"   (legacy bare basename from --command=chrome)
        //   "/usr/bin/firefox"  -> skip       (genuine host path; not a Flatpak entry)
        QString lookupKey;
        if (app.fullName.startsWith(QLatin1String("/app/"))) {
            lookupKey = QFileInfo(app.fullName).fileName();
            if (lookupKey.isEmpty()) continue;
        } else if (!app.fullName.startsWith('/')) {
            lookupKey = app.fullName;
        } else {
            continue;
        }

        if (!scanned) {
            commandToId = enumerateFlatpakCommandMap();
            scanned = true;
        }

        auto it = commandToId.find(lookupKey);
        if (it == commandToId.end()) continue;
        if (*it == app.fullName) continue;  // already migrated (mapped via .desktop stem to itself)

        qCDebug(LOG_BASIC) << "Migrating split-tunneling Flatpak entry:" << app.fullName << "->" << *it;
        app.fullName = *it;
    }
}

QString unescape(const QString &in)
{
    QString str = in;
    return str.replace("\\\"", "\"").replace("\\`", "`").replace("\\$", "$").replace("\\\\","\\");
}

QString convertToAbsolutePath(const QString &in)
{
    QFileInfo file(in);
    if (file.isAbsolute() && file.exists()) {
        return in;
    }

    QStringList paths = QString::fromLocal8Bit(qgetenv("PATH")).split(":");
    for (auto path : paths) {
        QString fullpath = path + "/" + in;
        QFileInfo file(fullpath);
        if (file.exists()) {
            return fullpath;
        }
    }
    return QString();
}

QString getDistroName()
{
    QString distro;

    QFile osReleaseFile("/etc/os-release");
    if (osReleaseFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&osReleaseFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("PRETTY_NAME=")) {
                distro = line.mid(12).removeFirst().removeLast();
            }
        }
    }

    if (distro.isEmpty()) {
        return "Unknown";
    }

    return distro;
}

bool isImmutableDistro() {
    // There isn't a perfect way to do this, but one good hint is if either / or /usr is mounted read-only.
    // Read the mounts from /proc/mounts
    QFile mountsFile("/proc/mounts");
    if (!mountsFile.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }

    QString contents = mountsFile.readAll();
    QTextStream in(&contents);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(' ');
        if (parts.size() < 3) {
            continue;
        }
        if (parts[1] == "/" || parts[1] == "/usr") {
            QRegularExpression re("(^ro$|^ro,|,ro,|,ro$)");
            QRegularExpressionMatch match = re.match(parts[3]);
            if (match.hasMatch()) {
                return true;
            }
        }
    }
    return false;
}

} // end namespace LinuxUtils
