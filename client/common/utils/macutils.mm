#import <Cocoa/Cocoa.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <AppKit/AppKit.h>

#include "logger.h"
#import "utils.h"
#import "macutils.h"
#include "names.h"
#include <QDir>
#include <QCoreApplication>
#include <libproc.h>
#include "executable_signature/executable_signature.h"

namespace {

QList<QString> listedCmdResults(const QString &cmd)
{
    QList<QString> result;
    QString response = Utils::execCmd(cmd);

    const QList<QString> lines = response.split("\n");
    for (const QString &line : lines)
    {
        const QString iName = line.trimmed();
        if (iName != "")
        {
            result.append(iName);
        }
    }

    return result;
}
}

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
    // Look for process containing "Windscribe" -- exclude grep and Engine
    QString cmd = "ps axco command | grep Windscribe | grep -v grep | grep -v WindscribeEngine | grep -v windscribe-cli";
    QString response = Utils::execCmd(cmd);
    return response.trimmed() != "";
}

QString MacUtils::iconPathFromBinPath(const QString &binPath)
{
    QString iconPath = "";
    const QString iconFolderPath = binPath + "/Contents/Resources/";
    const QString listIcnsCmd = "ls '" + iconFolderPath + "' | grep .icns";

    QList<QString> icons = listedCmdResults(listIcnsCmd);

    if (icons.length() > 0)
    {
        iconPath = iconFolderPath + icons[0];
    }
    return iconPath;
}

QList<QString> MacUtils::enumerateInstalledPrograms()
{
    QString cmd = "ls -d1 /Applications/* | grep .app";
    return listedCmdResults(cmd);
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
        if (appBundleId == QString(GUI_BUNDLE_ID))
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

void MacUtils::getNSWindowCenter(void *nsView, int &outX, int &outY)
{
    NSView *view = (__bridge NSView *)nsView;
    NSRect rc = view.window.frame;
    outX = rc.origin.x + rc.size.width / 2;
    outY = rc.origin.y + rc.size.height / 2;
}

bool MacUtils::dynamicStoreEntryHasKey(const QString &entry, const QString &key)
{
    SCDynamicStoreRef dynRef = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("WindscribeKeyChecker"), NULL, NULL);
    CFDictionaryRef dnskey = (CFDictionaryRef) SCDynamicStoreCopyValue(dynRef, entry.toCFString());
    CFStringRef setByWindscribeValue = (CFStringRef) CFDictionaryGetValue(dnskey, key.toCFString());
    CFRelease(dnskey);
    CFRelease(dynRef);
    return setByWindscribeValue != NULL;
}

bool MacUtils::verifyAppBundleIntegrity()
{
    // Following code adapted from genSignatureForFileAndArch in signature.mm of the OSQuery project.
#ifdef QT_DEBUG
    return true;
#else
    QString mainBundlePath = getBundlePath();

    qCDebug(LOG_BASIC) << "verifyAppBundleIntegrity on " << mainBundlePath;

    // Create a URL that points to this file.
    auto url = (__bridge CFURLRef)[NSURL fileURLWithPath:@(qPrintable(mainBundlePath))];
    if (url == nullptr)
    {
        qCDebug(LOG_BASIC) << "verifyAppBundleIntegrity: could not create URL from file";
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

        qCDebug(LOG_BASIC) << "verifyAppBundleIntegrity: could not create static code object";
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
        QString guiPath = QCoreApplication::applicationDirPath() + "/../../../../MacOS/Windscribe";
        guiPath = QDir::cleanPath(guiPath);

        if (parentPath.compare(guiPath, Qt::CaseInsensitive) == 0)
        {
            ExecutableSignature sigCheck;
            if (sigCheck.verify(parentPath.toStdWString())) {
                return true;
            }

            qCDebug(LOG_BASIC) << "isParentProcessGui incorrect signature: " << QString::fromStdString(sigCheck.lastError());
        }
    }
    return false;
}

bool MacUtils::isLockdownMode()
{
    QString response = Utils::execCmd("defaults read .GlobalPreferences.plist LDMGlobalEnabled");
    return response.trimmed() == "1";
}
