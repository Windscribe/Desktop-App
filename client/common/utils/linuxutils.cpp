#include "linuxutils.h"

#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QRegularExpression>
#include <QTextStream>

#include <grp.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "log/categories.h"

#include "utils.h"

namespace LinuxUtils
{

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
        QRegExp rx("(\\d+\\.\\d+(\\.\\d+)*)");
        if (rx.indexIn(unameData.release, 0) != -1) {
            return rx.cap(1);
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
        qCDebug(LOG_BASIC) << "Couldn't find previous install platform file: " << LAST_INSTALL_PLATFORM_FILE;
        return "";
    }

    QFile lastInstallPlatform(LAST_INSTALL_PLATFORM_FILE);

    if (!lastInstallPlatform.open(QIODevice::ReadOnly))
    {
        qCDebug(LOG_BASIC) << "Couldn't open previous install platform file: " << LAST_INSTALL_PLATFORM_FILE;
        return "";
    }

    linuxPlatformName = lastInstallPlatform.readAll().trimmed(); // remove newline
    return linuxPlatformName;
}

bool isAppAlreadyRunning()
{
    // Look for process containing "Windscribe" -- exclude grep and Engine
    QString cmd = "ps axco command | grep Windscribe | grep -v grep | grep -v WindscribeEngine | grep -v windscribe-cli";
    QString response = Utils::execCmd(cmd);
    return response.trimmed() != "";
}

QMap<QString, QString> enumerateInstalledPrograms()
{
    QMap<QString, QString> programs;

    // On Linux, we are looking for desktop entries. These are located under the applications dir at ~/.local/share, and each dir in $XDG_DATA_DIRS
    QStringList dirs = QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS")).split(":");
    dirs.prepend(QDir::homePath() + "/.local/share");

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

    qCDebug(LOG_BASIC) << "checking additional apps list";
    // also open the additional apps list so undetected apps can be added simply
    QFile f(QString(QDir::homePath() + "/.config/Windscribe/additionalapps.list"));
    if (f.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&f);
        QStringList contents = in.readAll().split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
        for(auto line: contents)
        {
            qCDebug(LOG_BASIC) << "added line for " << line;
            programs[line] = line;
        }
    }
    else {
        qCDebug(LOG_BASIC) << "couldn't open additional apps list" << QString(QDir::homePath() + ".config/Windscribe/additionalapps.list");
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

        if (isFlatpak && block.startsWith("--command=")) {
            return block.mid(10);
        }

        if (isSnap && block.startsWith("/snap/bin/")) {
            return block;
        }

        // ignore other lines with '='; they either set environment variables or are arguments
        if (block.contains("=")) {
            continue;
        }

        path = convertToAbsolutePath(block);
        if (!path.isEmpty()) {
            if (QFileInfo(QFile(path)).fileName() == "flatpak") {
                // This is a flatpak application.  Instead of passing the flatpak binary, get the actual command name.
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

gid_t getWindscribeGid()
{
    struct group *grp = getgrnam("windscribe");
    if (grp == nullptr) {
        return -1;
    }
    return grp->gr_gid;
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
