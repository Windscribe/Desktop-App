#import "installer.h"
#import <Cocoa/Cocoa.h>
#include <spdlog/spdlog.h>
#include "../helper/installhelper_mac.h"
#include "../string_utils.h"
#include "processes_helper.h"

@interface Installer()

-(void)execution;

@end

@implementation Installer


-(id)init
{
    if (self = [super init]) {
        d_group_ = nil;
    }
    return self;
}

-(id)initWithPath: (NSString *)path
{
    if (self = [super initWithUpdatePath: path]) {
        d_group_ = nil;
    }
    return self;
}

-(void)setCallback: (std::function<void()>)func
{
    callback_ = func;
}

-(BOOL)isFolderAlreadyExist
{
    if (isUseUpdatePath_) {
        return NO;
    } else {
        NSFileManager *manager = [NSFileManager defaultManager];
        return [manager fileExistsAtPath:[self getOldInstallPath] isDirectory:nil];
    }
}

-(void)start
{
    spdlog::info("Installer starting");

    isCanceled_ = false;

    d_group_ = dispatch_group_create();
    dispatch_queue_t bg_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    dispatch_group_async(d_group_, bg_queue, ^{
        [self execution];
    });
}

// cancel and do uninstall
-(void)cancel
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isCanceled_ = true;
        self.currentState = STATE_CANCELED;
    }
    [self waitForCompletion];

    [[NSFileManager defaultManager] removeItemAtPath:[self getOldInstallPath] error:nil];
}

-(void)runLauncher
{
    if([[NSWorkspace sharedWorkspace] openURL: [NSURL fileURLWithPath: [self getInstallPath]]] == NO) {
        spdlog::error("App failed to launch");
    }
    self.progress = 100;
    self.currentState = STATE_LAUNCHED;
    callback_();
}

-(NSString *)runProcess:(NSString*)exePath args:(NSArray *)args
{
    NSPipe *pipe = [[NSPipe alloc] init];
    NSFileHandle *file = [pipe fileHandleForReading];

    NSTask *task = [[NSTask alloc] init];
    task.launchPath = exePath;
    task.arguments = args;
    task.standardOutput = pipe;
    @try {
        [task launch];

        NSData *data = [file readDataToEndOfFile];
        [file closeFile];
        return [[NSString alloc] initWithData: data encoding: NSUTF8StringEncoding];
    }
    @catch (NSException *e) {
        spdlog::error("Exception occurred {}", toStdString([e reason]));
        [file closeFile];
        return nil;
    }
}

-(void) execution
{
    bool terminated = false;
    self.progress = 0;
    spdlog::info("Starting execution");
    self.currentState = STATE_EXTRACTING;
    callback_();

    ProcessesHelper processesHelper;

    // kill processes with "Windscribe" name
    std::vector<pid_t> processesList = processesHelper.getPidsByProcessname("Windscribe");
    // get WindscribeEngine processes
    std::vector<pid_t> engineList = processesHelper.getPidsByProcessname("WindscribeEngine");
    processesList.insert(processesList.end(), engineList.begin(), engineList.end());

    if (processesList.size() > 0) {
        // try to terminate Windscribe processes
        spdlog::info("Waiting for Windscribe programs to close...");

        // first send SIGTERM signal
        for (auto pid : processesList) {
          kill(pid, SIGTERM);
        }
        terminated = [self waitForProcessFinish:processesList helper:&processesHelper timeoutSec:5];

        // if SIGTERM didn't kill processes, then send SIGKILL
        if (!terminated) {
            for (auto pid : processesList) {
              kill(pid, SIGKILL);
            }
            terminated = [self waitForProcessFinish:processesList helper:&processesHelper timeoutSec:5];
        }

        // Still could not terminate, return error
        if (!terminated) {
            spdlog::error("Couldn't kill running Windscribe programs in time. Please close running Windscribe programs manually and try install again.");
            self.lastError = ERROR_KILL;
            self.currentState = STATE_ERROR;
            callback_();
            return;
        }

        spdlog::info("All Windscribe programs closed");
    }

    NSString *disabledList = [self runProcess:@"/bin/launchctl" args:@[@"print-disabled", @"system"]];
    if (disabledList == nil) {
        spdlog::info("Couldn't detect if the helper was disabled.");
    } else {
        if ([disabledList rangeOfString:@"\"com.windscribe.helper.macos\" => disabled"].location != NSNotFound ||
            [disabledList rangeOfString:@"\"com.windscribe.helper.macos\" => true"].location != NSNotFound) {

            // If somehow launchctl has previously disabled our helper, we won't be able to install it again.  Enable it.
            spdlog::info("Helper is disabled. Re-enabling.");
            NSString *scriptContents = @"do shell script \"launchctl enable system/com.windscribe.helper.macos\" with administrator privileges";
            NSAppleScript *script = [[NSAppleScript alloc] initWithSource:scriptContents];
            NSAppleEventDescriptor *desc;
            desc = [script executeAndReturnError:nil];
            if (desc == nil) {
                spdlog::error("Couldn't re-enable the helper.");
                self.lastError = ERROR_OTHER;
                self.currentState = STATE_ERROR;
                callback_();
                return;
            }
        }
    }

    // Install new helper now that we are sure the client app has exited. Otherwise we may cause the
    // client app to hang when we pull the old helper out from under it.
    if (!InstallHelper_mac::installHelper(self.factoryReset)) {
        spdlog::error("Couldn't install the helper.");
        self.lastError = ERROR_PERMISSION;
        self.currentState = STATE_ERROR;
        callback_();
        return;
    }

    if (!self.connectHelper) {
        spdlog::error("Couldn't connect to new helper in time");
        self.lastError = ERROR_CONNECT_HELPER;
        self.currentState = STATE_ERROR;
        callback_();
        return;
    }

    // remove previously existing application
    if ([self isFolderAlreadyExist] || isUseUpdatePath_) {
        NSString *bundleVersion = [self runProcess:@"/usr/bin/mdls" args:@[@"-name", @"kMDItemVersion", [self getOldInstallPath]]];
        spdlog::info("Windscribe exists in desired folder, {}", toStdString(bundleVersion));
        spdlog::info("Attempting to remove: {}", toStdString([self getOldInstallPath]));

        bool success = helper_.removeOldInstall([[self getOldInstallPath] UTF8String]);
        if (!success) {
            spdlog::error("Previous version of the program cannot be deleted. Please contact support.");
            self.lastError = ERROR_DELETE;
            self.currentState = STATE_ERROR;
            callback_();
            helper_.stop();
            return;
        }

        spdlog::info("Removed!");
    }

    if (self.factoryReset) {
        spdlog::info("Executing factory reset");

        // NB: do not execute these as root
        NSMutableString *path = [NSMutableString stringWithString:NSHomeDirectory()];
        [path appendString:@"/Library/Preferences/com.windscribe.Windscribe.plist"];
        [self runProcess:@"/bin/rm" args:@[path]];

        [path setString:NSHomeDirectory()];
        [path appendString:@"/Library/Preferences/com.windscribe.Windscribe2.plist"];
        [self runProcess:@"/bin/rm" args:@[path]];

        [path setString:NSHomeDirectory()];
        [path appendString:@"/Library/Preferences/com.windscribe.gui.macos.plist"];
        [self runProcess:@"/bin/rm" args:@[path]];

        [path setString:NSHomeDirectory()];
        [path appendString:@"/Library/Application Support/windscribe_extra.conf"];
        [self runProcess:@"/bin/rm" args:@[path]];

        [path setString:NSHomeDirectory()];
        [path appendString:@"/Library/Application Support/Windscribe/Windscribe2"];
        [self runProcess:@"/bin/rm" args:@[@"-r", path]];

        [self runProcess:@"/usr/bin/killall" args:@[@"cfprefsd"]];
    }

    spdlog::info("Extracting and installing Windscribe app");

    NSString* archivePathFromApp = [[NSBundle mainBundle] pathForResource:@"windscribe.tar.lzma" ofType:nil];
    std::wstring strArchivePath = NSStringToStringW(archivePathFromApp);
    std::wstring strPath = NSStringToStringW([self getInstallPath]);

    self.progress = 50;
    self.currentState = STATE_EXTRACTING;
    callback_();

    if (!helper_.setPaths(strArchivePath, strPath)) {
        spdlog::error("setPaths in helper failed");
        self.lastError = ERROR_OTHER;
        self.currentState = STATE_ERROR;
        callback_();
        helper_.stop();
        return;
    }

    if (!helper_.executeFilesStep()) {
        spdlog::error("executeFilesStep in helper failed");
        self.lastError = ERROR_OTHER;
        self.currentState = STATE_ERROR;
        callback_();
        helper_.stop();
        return;
    }

    spdlog::info("Windscribe app installed");

    self.progress = 75;
    self.currentState = STATE_EXTRACTED;
    callback_();

    helper_.createCliSymlink();

    self.progress = 90;
    self.currentState = STATE_FINISHED;
    callback_();

    helper_.stop();
}

-(void)waitForCompletion
{
    if (d_group_ != nil)
    {
        dispatch_group_wait(d_group_, DISPATCH_TIME_FOREVER);
        d_group_ = nil;
    }
}

std::wstring NSStringToStringW ( NSString* Str )
{
    NSStringEncoding pEncode    =   CFStringConvertEncodingToNSStringEncoding ( kCFStringEncodingUTF32LE );
    NSData* pSData              =   [ Str dataUsingEncoding : pEncode ];

    return std::wstring ( (wchar_t*) [ pSData bytes ], [ pSData length] / sizeof ( wchar_t ) );
}

NSString* StringWToNSString ( const std::wstring& Str )
{
    NSString* pString = [ [ NSString alloc ]
                        initWithBytes : (char*)Str.data()
                               length : Str.size() * sizeof(wchar_t)
                             encoding : CFStringConvertEncodingToNSStringEncoding ( kCFStringEncodingUTF32LE ) ];
    return pString;
}

- (BOOL)connectHelper
{
    if (helper_.connect())
      return YES;
    else
      return NO;
}

- (BOOL)waitForProcessFinish: (std::vector<pid_t>)processes helper:(ProcessesHelper *)helper timeoutSec:(size_t)timeoutSec
{
    NSDate *waitingSince_ = [NSDate date];
    while (true) {
        bool bAllFinished = true;
        for (auto pid : processes) {
            if (!helper->isProcessFinished(pid)) {
                bAllFinished = false;
                break;
            }
        }

        if (bAllFinished) {
            break;
        }

        usleep(10000); // 10 milliseconds
        int seconds = -(int)[waitingSince_ timeIntervalSinceNow];
        if (seconds > timeoutSec) {
            return false;
        }
    }
    return true;
}

@end
