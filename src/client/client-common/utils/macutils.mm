#import <Cocoa/Cocoa.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <AppKit/AppKit.h>

#include <QCoreApplication>
#include <QDir>
#include <QHostAddress>
#include <QScopeGuard>

#include <dirent.h>
#include <libproc.h>
#include <limits.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <vector>

#include "executable_signature/executable_signature.h"
#include "log/categories.h"
#include "macutils.h"
#include "utils.h"

void MacUtils::activateApp()
{
    [NSApp activateIgnoringOtherApps:YES];
}

void MacUtils::invalidateShadow(void *pNSView)
{
    NSView *view = (__bridge NSView *)pNSView;
    NSWindow *win = [view window];
    [win invalidateShadow];
}

void MacUtils::invalidateCursorRects(void *pNSView)
{
    NSView *view = (__bridge NSView *)pNSView;
    NSWindow *win = [view window];
    [win resetCursorRects];
}

void MacUtils::getOSVersionAndBuild(QString &osVersion, QString &build)
{
    osVersion = Utils::execCmd("sw_vers -productVersion").trimmed();
    build = Utils::execCmd("sw_vers -buildVersion").trimmed();
}

QString MacUtils::getOsVersion()
{
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    NSString *osVer = processInfo.operatingSystemVersionString;
    return QString::fromCFString((__bridge CFStringRef)osVer);
}

bool MacUtils::isOsVersionAtLeast(int major, int minor)
{
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    NSOperatingSystemVersion version = { major, minor, 0 };
    return [processInfo isOperatingSystemAtLeastVersion:version];
}

void MacUtils::hideDockIcon()
{
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToUIElementApplication);
}

void MacUtils::showDockIcon()
{
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}

void MacUtils::setHandCursor()
{
    [[NSCursor pointingHandCursor] set];
}

void MacUtils::setArrowCursor()
{
    [[NSCursor arrowCursor] set];
}

bool MacUtils::isAppAlreadyRunning()
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

// getfsstat(MNT_NOWAIT) returns the kernel's cached mount records without asking each filesystem to
// refresh, so it cannot stall on a dead mount. The prefix match is lexical for the same reason:
// stat'ing the path would trigger automounts and hang on exactly the volumes this exists to avoid.
static bool clearOfNonLocalMounts(const QString &path, bool checkBelow)
{
    int count = getfsstat(nullptr, 0, MNT_NOWAIT);
    if (count <= 0) {
        return false;
    }
    // Over-allocate so a volume mounted between the two calls cannot be silently truncated away.
    std::vector<struct statfs> mounts(count + 8);
    count = getfsstat(mounts.data(), static_cast<int>(mounts.size() * sizeof(struct statfs)), MNT_NOWAIT);
    if (count <= 0) {
        return false;
    }

    const QString p = path.endsWith('/') ? path : path + '/';
    for (int i = 0; i < count; i++) {
        if ((mounts[i].f_flags & MNT_LOCAL) && strcmp(mounts[i].f_fstypename, "autofs") != 0) {
            continue;
        }
        const QString mountPoint = QString::fromUtf8(mounts[i].f_mntonname);
        const QString prefix = mountPoint.endsWith('/') ? mountPoint : mountPoint + '/';
        // Every mount the path traverses must be local, not just the deepest: reaching a local volume
        // mounted beneath a network directory still walks through the dead parent. Case-insensitive
        // because the default filesystem is, and over-matching can only add refusals.
        if (p.startsWith(prefix, Qt::CaseInsensitive)) {
            return false;
        }
        // A mount below the path sits where bundle reads descend, so it is refused the same way.
        if (checkBelow && prefix.startsWith(p, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

// System paths need two symlink hops (/Applications/Safari.app -> /System/Cryptexes/App -> the Preboot
// cryptex); 8 is a conservative budget across all links in one path, far past any real layout.
static constexpr int kMaxSymlinkHops = 8;

static bool isOnLocalVolumeImpl(const QString &path, int hops, bool checkBelow)
{
    // Only canonical-form absolute paths: '.', '..', doubled or trailing separators make the lexical
    // mount match below diverge from what traversal actually crosses. cleanPath is purely lexical --
    // and it keeps above-root ".." (the kernel clamps "/.." to "/"), so a cleaned path can lead with it.
    if (!path.startsWith('/') || path != QDir::cleanPath(path) || path == "/.." || path.startsWith("/../")) {
        return false;
    }

    if (!clearOfNonLocalMounts(path, checkBelow)) {
        return false;
    }

    // A symlink component can route a lexically local path onto a network volume; expand it lexically
    // (readlink reads the link inode, never the target) and re-prove the rewritten path from the mount
    // table up. Every prefix probed here is already proven local, so lstat/readlink cannot stall.
    const QByteArray native = path.toUtf8();
    for (qsizetype pos = native.indexOf('/', 1); ; pos = native.indexOf('/', pos + 1)) {
        const QByteArray prefix = (pos == -1) ? native : native.left(pos);
        struct stat st;
        if (lstat(prefix.constData(), &st) != 0) {
            // Absent paths are safe to accept -- the consumer's own stat fails just as fast. Any other
            // failure (ENAMETOOLONG on an over-expanded rewrite, EACCES) leaves the tail unproven: refuse.
            return errno == ENOENT || errno == ENOTDIR;
        }
        if (S_ISLNK(st.st_mode)) {
            if (hops <= 0) {
                return false;
            }
            char target[PATH_MAX];
            const ssize_t len = readlink(prefix.constData(), target, sizeof(target) - 1);
            if (len <= 0) {
                return false;
            }
            QString targetPath = QString::fromUtf8(target, len);
            // Link content is raw bytes: if it doesn't round-trip through UTF-16, the path proven below
            // is not the one the kernel resolves.
            if (targetPath.toUtf8() != QByteArray(target, static_cast<qsizetype>(len))) {
                return false;
            }
            // A "." collapses in place and a leading ".." run in a relative target climbs only components
            // already proven non-symlink, so both collapse faithfully. Any other ".." collapses across an
            // unproven component the kernel enters first (following links, crossing mounts) -- refuse.
            const QStringList parts = targetPath.split('/', Qt::SkipEmptyParts);
            qsizetype firstReal = 0;
            if (!targetPath.startsWith('/')) {
                while (firstReal < parts.size() && (parts[firstReal] == "." || parts[firstReal] == "..")) {
                    firstReal++;
                }
            }
            for (qsizetype i = firstReal; i < parts.size(); i++) {
                if (parts[i] == "..") {
                    return false;
                }
            }
            if (!targetPath.startsWith('/')) {
                targetPath = QString::fromUtf8(prefix.left(prefix.lastIndexOf('/'))) + '/' + targetPath;
            }
            const QString remainder = (pos == -1) ? QString() : QString::fromUtf8(native.mid(pos));
            // One cleanPath over the whole join: a target resolving to "/" would otherwise double the slash.
            return isOnLocalVolumeImpl(QDir::cleanPath(targetPath + remainder), hops - 1, checkBelow);
        }
        if (pos == -1) {
            return true;
        }
    }
}

bool MacUtils::isOnLocalVolume(const QString &path)
{
    return isOnLocalVolumeImpl(path, kMaxSymlinkHops, true);
}

QString MacUtils::iconPathFromBinPath(const QString &binPath)
{
    if (!isOnLocalVolume(binPath)) {
        return QString();
    }

    // Own pool: this runs on a worker thread that has no event loop, so nothing else drains one.
    @autoreleasepool {
        NSBundle *bundle = [NSBundle bundleWithPath:binPath.toNSString()];
        if (bundle != nil) {
            NSString *iconFile = [[bundle infoDictionary] objectForKey:@"CFBundleIconFile"];
            if ([iconFile isKindOfClass:[NSString class]] && [iconFile length] > 0) {
                NSString *resolved = [bundle pathForResource:iconFile ofType:@"icns"];
                if (resolved == nil) {
                    resolved = [bundle pathForResource:iconFile ofType:nil];
                }
                if (resolved != nil) {
                    return QString::fromNSString(resolved);
                }
            }
        }
    }

    // Apps that declare their icon in an asset catalog (CFBundleIconName) have no Info.plist path to follow.
    const QDir resourcesDir(binPath + "/Contents/Resources");
    const QStringList icons = resourcesDir.entryList(QStringList("*.icns"), QDir::Files, QDir::Name);
    if (!icons.isEmpty()) {
        return resourcesDir.filePath(icons.first());
    }

    return QString();
}

QList<QString> MacUtils::enumerateInstalledPrograms()
{
    QList<QString> apps;

    // Finder's Applications view is a merge: Apple's stock apps (App Store, Mail, ...) actually live
    // in /System/Applications and its Utilities folder.
    for (const char *root : {"/Applications", "/Applications/Utilities", "/System/Applications", "/System/Applications/Utilities"}) {
        // /Applications/Utilities is user-created territory where a symlink or dead mount would park
        // opendir itself. Roots are only listed, never descended, and each entry is re-proven below,
        // so a mount beneath the root must not refuse it.
        if (!isOnLocalVolumeImpl(QString::fromUtf8(root), kMaxSymlinkHops, false)) {
            continue;
        }
        // QDir's type filters classify every entry, and classification stats through a symlink -- hanging on
        // a link into a dead mount before the loop could refuse it. readdir lists names and touches nothing.
        DIR *dir = opendir(root);
        if (dir == nullptr) {
            continue;
        }
        QStringList entries;
        while (struct dirent *entry = readdir(dir)) {
            const QString name = QString::fromUtf8(entry->d_name);
            if (name.endsWith(".app", Qt::CaseInsensitive) && !name.startsWith('.')) {
                entries << name;
            }
        }
        closedir(dir);

        for (const QString &entry : entries) {
            const QString entryPath = QString::fromUtf8(root) + '/' + entry;
            // Proves every hop local from the mount table before anything here stats the entry; symlinked
            // entries (Safari.app -> the App cryptex) are expanded and re-proven inside.
            if (!isOnLocalVolume(entryPath)) {
                continue;
            }
            const QFileInfo info(entryPath);
            if (!info.isDir()) {
                continue;
            }
            // De-dup on the canonical path: a user symlink in /Applications to a /System/Applications app
            // lands here from both roots.
            const QString path = QString::fromStdString(info.filesystemCanonicalFilePath());
            if (!path.isEmpty() && !apps.contains(path)) {
                apps << path;
            }
        }
    }

    // The multi-root merge and canonicalization (cryptex paths) both scramble name order, and the picker
    // shows rows in list order, named with ".app" stripped -- sort on that key, path tiebreak for determinism.
    auto displayName = [](const QString &path) {
        QString name = QFileInfo(path).fileName();
        if (name.endsWith(".app", Qt::CaseInsensitive)) {
            name.chop(4);
        }
        return name;
    };
    std::sort(apps.begin(), apps.end(), [&displayName](const QString &a, const QString &b) {
        const int cmp = displayName(a).compare(displayName(b), Qt::CaseInsensitive);
        return (cmp != 0) ? cmp < 0 : a < b;
    });
    return apps;
}

NSRunningApplication *guiApplicationByBundleName()
{
    NSRunningApplication *currentApp = [NSRunningApplication currentApplication];
    NSWorkspace * ws = [NSWorkspace sharedWorkspace];
    NSArray * apps = [ws runningApplications];

    NSUInteger count = [apps count];
    for (NSUInteger i = 0; i < count; i++)
    {
        NSRunningApplication *app = [apps objectAtIndex: i];
        QString appBundleId = QString::fromNSString([app bundleIdentifier]);
        if (appBundleId == QString(WS_MAC_GUI_BUNDLE_ID))
        {
            if ([app processIdentifier] != [currentApp processIdentifier])
            {
                return app;
            }
        }
    }

    return NULL;
}

bool MacUtils::showGui()
{
    NSRunningApplication *guiApp = guiApplicationByBundleName();
    if (guiApp != NULL)
    {
        [guiApp activateWithOptions: NSApplicationActivateAllWindows];
        [guiApp unhide];
        return true;
    }
    return false;
}

QString MacUtils::getBundlePath()
{
    return QString::fromNSString([[NSBundle mainBundle] bundlePath]);
}

bool MacUtils::hasActiveDisplay()
{
    uint32_t displayCount = 0;
    if (CGGetActiveDisplayList(0, nullptr, &displayCount) != kCGErrorSuccess) {
        return false;
    }
    return displayCount > 0;
}

void MacUtils::getNSWindowCenter(void *nsView, int &outX, int &outY)
{
    NSView *view = (__bridge NSView *)nsView;
    NSRect rc = view.window.frame;
    outX = rc.origin.x + rc.size.width / 2;
    outY = rc.origin.y + rc.size.height / 2;
}

bool MacUtils::dynamicStoreEntryHasKey(const QString &entry, const QString &key)
{
    SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("" WS_APP_IDENTIFIER "KeyChecker"), NULL, NULL);
    if (dynRef == NULL) {
        qCCritical(LOG_BASIC) << "dynamicStoreEntryHasKey - SCDynamicStoreCreate failed";
        return false;
    }

    CFStringRef setByAppValue = NULL;
    CFStringRef entryCFString = entry.toCFString();
    CFStringRef keyCFString = key.toCFString();
    CFDictionaryRef dnskey = (CFDictionaryRef) SCDynamicStoreCopyValue(dynRef, entryCFString);
    if (dnskey != NULL) {
        setByAppValue = (CFStringRef) CFDictionaryGetValue(dnskey, keyCFString);
        CFRelease(dnskey);
    } else {
        qCCritical(LOG_BASIC) << "dynamicStoreEntryHasKey - SCDynamicStoreCopyValue failed";
    }
    CFRelease(dynRef);
    CFRelease(entryCFString);
    CFRelease(keyCFString);
    return setByAppValue != NULL;
}

bool MacUtils::verifyAppBundleIntegrity()
{
    // Following code adapted from genSignatureForFileAndArch in signature.mm of the OSQuery project.
#ifndef USE_SIGNATURE_CHECK
    return true;
#else
    QString mainBundlePath = getBundlePath();

    // Create a URL that points to this file.
    auto url = (__bridge CFURLRef)[NSURL fileURLWithPath:@(qPrintable(mainBundlePath))];
    if (url == nullptr)
    {
        qCCritical(LOG_BASIC) << "verifyAppBundleIntegrity: could not create URL from file";
        return false;
    }

    // Create the static code object.
    SecStaticCodeRef static_code = nullptr;
    OSStatus result = SecStaticCodeCreateWithPath(url, kSecCSDefaultFlags, &static_code);

    if (result != errSecSuccess)
    {
        if (static_code != nullptr)
        {
            CFRelease(static_code);
        }

        qCCritical(LOG_BASIC) << "verifyAppBundleIntegrity: could not create static code object";
        return false;
    }

    SecCSFlags flags = kSecCSStrictValidate | kSecCSCheckAllArchitectures | kSecCSCheckNestedCode;
    result = SecStaticCodeCheckValidityWithErrors(static_code, flags, nullptr, nullptr);

    CFRelease(static_code);

    qCDebug(LOG_BASIC) << "verifyAppBundleIntegrity completed successfully";

    return (result == errSecSuccess);
#endif
}

bool MacUtils::isParentProcessGui()
{
    pid_t pid = getppid();
    char pathBuffer[PROC_PIDPATHINFO_MAXSIZE] = {0};
    int status = proc_pidpath(pid, pathBuffer, sizeof(pathBuffer));
    if ((status != 0) && (strlen(pathBuffer) != 0))
    {
        QString parentPath = QString::fromStdString(pathBuffer);
        QString guiPath = QCoreApplication::applicationDirPath() + "/../../../../MacOS/" WS_APP_EXECUTABLE_NAME;
        guiPath = QDir::cleanPath(guiPath);

        if (parentPath.compare(guiPath, Qt::CaseInsensitive) == 0)
        {
            ExecutableSignature sigCheck;
            if (sigCheck.verify(parentPath.toStdWString())) {
                return true;
            }

            qCCritical(LOG_BASIC) << "isParentProcessGui incorrect signature: " << QString::fromStdString(sigCheck.lastError());
        }
    }
    return false;
}

bool MacUtils::isLockdownMode()
{
    QString response = Utils::execCmd("defaults read .GlobalPreferences.plist LDMGlobalEnabled");
    return response.trimmed() == "1";
}

static QStringList getOsDnsServersFromPath(CFStringRef path)
{
    QStringList servers;

    SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("DNSSETTING"), NULL, NULL);
    if (dynRef == NULL) {
        qCCritical(LOG_BASIC) << "getOsDnsServersFromPath - SCDynamicStoreCreate failed";
        return servers;
    }

    CFPropertyListRef propList = NULL;
    auto exitGuard = qScopeGuard([&] {
        if (propList != NULL) {
            CFRelease(propList);
        }
        CFRelease(dynRef);
    });

    propList = SCDynamicStoreCopyValue(dynRef, path);
    if (propList) {
        CFDictionaryRef dict = (CFDictionaryRef)propList;
        CFArrayRef addresses = (CFArrayRef)CFDictionaryGetValue(dict, CFSTR("ServerAddresses"));
        if (addresses == NULL) {
            qCCritical(LOG_BASIC) << "getOsDnsServersFromPath - CFDictionaryGetValue failed";
            return servers;
        }
        for (int j = 0; j < CFArrayGetCount(addresses); j++) {
            NSString *addr = (NSString *)CFArrayGetValueAtIndex(addresses, j);
            servers << QString([addr UTF8String]);
        }
    }

    return servers;
}

QSet<QString> MacUtils::getOsDnsServers()
{
    SCPreferencesRef prefsDNS = SCPreferencesCreate(NULL, CFSTR("DNSSETTING"), NULL);
    if (prefsDNS == NULL) {
        qCCritical(LOG_BASIC) << "getOsDnsServers - SCPreferencesCreate failed";
        return QSet<QString>();
    }

    CFArrayRef services = NULL;
    auto exitGuard = qScopeGuard([&] {
        if (services != NULL) {
            CFRelease(services);
        }
        CFRelease(prefsDNS);
    });

    services = SCNetworkServiceCopyAll(prefsDNS);
    if (services == NULL) {
        qCCritical(LOG_BASIC) << "getOsDnsServers - SCNetworkServiceCopyAll failed";
        return QSet<QString>();
    }

    QStringList servers;
    for (long i = 0; i < CFArrayGetCount(services); i++) {
        const SCNetworkServiceRef service = (const SCNetworkServiceRef)CFArrayGetValueAtIndex(services, i);
        CFStringRef serviceId = SCNetworkServiceGetServiceID(service);
        CFStringRef path = CFStringCreateWithFormat(NULL, NULL, CFSTR("State:/Network/Service/%@/DNS"), serviceId);
        if (path == NULL) {
            qCCritical(LOG_BASIC) << "getOsDnsServers - CFStringCreateWithFormat(Service) failed";
            return QSet<QString>();
        }
        servers << getOsDnsServersFromPath(path);
        CFRelease(path);
    }

    CFStringRef globalPath = CFStringCreateWithFormat(NULL, NULL, CFSTR("State:/Network/Global/DNS"));
    if (globalPath == NULL) {
        qCCritical(LOG_BASIC) << "getOsDnsServers - CFStringCreateWithFormat(Global) failed";
        return QSet<QString>();
    }

    servers << getOsDnsServersFromPath(globalPath);
    CFRelease(globalPath);

    QSet<QString> result(servers.begin(), servers.end());

    // Diagnostic: split v4/v6 counts so a misclassified or missing IPv6 system resolver shows up
    // in logs. `MacUtils::getOsDnsServers()` feeds `firewallcontroller_mac.cpp`'s `<disallowed_dns>`
    // pf table — if a host configured with a v6 ISP DNS server logs `ipv6=0` here, the firewall
    // cannot block it and a v6 DNS leak is possible. macOS stores both families in a single flat
    // `ServerAddresses` array, so dual-stack hosts should always show non-zero on both counters.
    int v4Count = 0;
    int v6Count = 0;
    for (const QString &s : std::as_const(result)) {
        QHostAddress addr(s);
        if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
            ++v6Count;
        } else if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            ++v4Count;
        }
    }
    qCDebug(LOG_BASIC) << "getOsDnsServers - found" << result.size()
                       << "OS DNS resolver(s):" << v4Count << "IPv4," << v6Count << "IPv6";

    return result;
}
