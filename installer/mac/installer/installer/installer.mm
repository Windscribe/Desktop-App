#import "installer.h"
#import "../Logger.h"
#include "../helper/installhelper_mac.h"
#include "processes_helper.h"

@interface Installer()

-(void)execution;

@end

@implementation Installer


-(id)init
{
    if (self = [super init])
    {
        d_group_ = nil;
    }
    return self;
}

-(id)initWithPath: (NSString *)path
{
    if (self = [super initWithUpdatePath: path])
    {
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
    [[Logger sharedLogger] logAndStdOut:@"Installer starting"];

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

- (void)appDidLaunch:(NSNotification*)note
{
    self.progress = 100;
    self.currentState = STATE_LAUNCHED;
    callback_();
}

-(void)runLauncher
{
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(appDidLaunch:) name:NSWorkspaceDidLaunchApplicationNotification object:nil];
    NSWorkspaceOpenConfiguration *config = [NSWorkspaceOpenConfiguration configuration];
    [config setCreatesNewApplicationInstance: YES];
    [[NSWorkspace sharedWorkspace] openApplicationAtURL: [NSURL fileURLWithPath: [self getInstallPath]] configuration: config completionHandler: nil];
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
        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Exception occurred %@", [e reason]]];
        [file closeFile];
        return nil;
    }
}

-(void) execution
{
    int prevOverallProgress = 0;
    bool terminated = false;
    self.progress = 0;
    [[Logger sharedLogger] logAndStdOut:@"Starting execution"];
    self.currentState = STATE_EXTRACTING;
    callback_();

    BOOL connectedOldHelper = NO;

    ProcessesHelper processesHelper;
    std::vector<pid_t> processesList = processesHelper.getPidsByProcessname("com.windscribe.helper.macos");
    if (processesList.size() > 0) {
        connectedOldHelper = self.connectHelper;
    }

    // kill processes with "Windscribe" name
    processesList = processesHelper.getPidsByProcessname("Windscribe");
    // get WindscribeEngine processes
    std::vector<pid_t> engineList = processesHelper.getPidsByProcessname("WindscribeEngine");
    processesList.insert(processesList.end(), engineList.begin(), engineList.end());

    if (processesList.size() > 0) {
        // try to terminate Windscribe processes with new helper interface
        if (connectedOldHelper) {
            helper_.killWindscribeProcess();
        }
        terminated = [self waitForProcessFinish:processesList helper:&processesHelper timeoutSec:5];

        if (!terminated) {
            // if the above method failed or helper was not connected, try the older way
            [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Waiting for Windscribe programs to close..."]];

            for (auto pid : processesList) {
                if (connectedOldHelper) {
                    helper_.killProcess(pid);
                } else {
                    kill(pid, SIGTERM);
                }
            }
            terminated = [self waitForProcessFinish:processesList helper:&processesHelper timeoutSec:5];

            // Still could not terminate, return error
            if (!terminated) {
                NSString *errStr = @"Couldn't kill running Windscribe programs in time. Please close running Windscribe programs manually and try install again.";
                [[Logger sharedLogger] logAndStdOut:errStr];
                self.lastError = ERROR_KILL;
                self.currentState = STATE_ERROR;
                callback_();
                return;
            }
        }

        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"All Windscribe programs closed"]];
    }

    if (self.factoryReset && connectedOldHelper)
    {
        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Deleting old helper"]];
        helper_.deleteOldHelper();
    }

    helper_.stop();

    NSString *disabledList = [self runProcess:@"/bin/launchctl" args:@[@"print-disabled", @"system"]];
    if (disabledList == nil) {
        [[Logger sharedLogger] logAndStdOut:@"Couldn't detect if the helper was disabled."];
    } else {
        if ([disabledList rangeOfString:@"\"com.windscribe.helper.macos\" => disabled"].location != NSNotFound ||
            [disabledList rangeOfString:@"\"com.windscribe.helper.macos\" => true"].location != NSNotFound) {

            // If somehow launchctl has previously disabled our helper, we won't be able to install it again.  Enable it.
            [[Logger sharedLogger] logAndStdOut:@"Helper is disabled. Re-enabling."];
            NSString *scriptContents = @"do shell script \"launchctl enable system/com.windscribe.helper.macos\" with administrator privileges";
            NSAppleScript *script = [[NSAppleScript alloc] initWithSource:scriptContents];
            NSAppleEventDescriptor *desc;
            desc = [script executeAndReturnError:nil];
            if (desc == nil) {
                NSString *errStr = @"Couldn't re-enable the helper.";
                [[Logger sharedLogger] logAndStdOut:errStr];
                self.lastError = ERROR_OTHER;
                self.currentState = STATE_ERROR;
                callback_();
                return;
            }
        }
    }

    // Install new helper now that we are sure the client app has exited. Otherwise we may cause the
    // client app to hang when we pull the old helper out from under it.
    if (!InstallHelper_mac::installHelper())
    {
        NSString *errStr = @"Couldn't install the helper.";
        [[Logger sharedLogger] logAndStdOut:errStr];
        self.lastError = ERROR_PERMISSION;
        self.currentState = STATE_ERROR;
        callback_();
        return;
    }

    if (!self.connectHelper)
    {
        NSString *errStr = @"Couldn't connect to new helper in time";
        [[Logger sharedLogger] logAndStdOut:errStr];
        self.lastError = ERROR_CONNECT_HELPER;
        self.currentState = STATE_ERROR;
        callback_();
        return;
    }

    // remove previously existing application
    if ([self isFolderAlreadyExist] || isUseUpdatePath_)
    {
        [[Logger sharedLogger] logAndStdOut:@"Windscribe exists in desired folder"];

        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Attempting to remove: %@", [self getOldInstallPath]]];

        bool success = helper_.removeOldInstall([[self getOldInstallPath] UTF8String]);
        if (!success)
        {
            NSString *errStr = @"Previous version of the program cannot be deleted. Please contact support.";
            [[Logger sharedLogger] logAndStdOut:errStr];
            self.lastError = ERROR_DELETE;
            self.currentState = STATE_ERROR;
            callback_();
            helper_.stop();
            return;
        }
        else
        {
            [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Removed!"]];
        }
    }

    if (self.factoryReset)
    {
        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Executing factory reset"]];

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

    [[Logger sharedLogger] logAndStdOut:@"Writing blocks"];

    uid_t userId = getuid();
    gid_t groupId = getgid();

    NSString* archivePathFromApp = [[NSBundle mainBundle] pathForResource:@"windscribe.7z" ofType:nil];
    std::wstring strArchivePath = NSStringToStringW(archivePathFromApp);
    std::wstring strPath = NSStringToStringW([self getInstallPath]);

    if (!helper_.setPaths(strArchivePath, strPath, userId, groupId))
    {
        NSString *errStr = @"setPaths in helper failed";
        [[Logger sharedLogger] logAndStdOut:errStr];
        self.lastError = ERROR_OTHER;
        self.currentState = STATE_ERROR;
        callback_();
        helper_.stop();
        return;
    }

    while (true)
    {
        //[NSThread sleepForTimeInterval:0.2];
        std::lock_guard<std::mutex> lock(mutex_);
        if (isCanceled_)
        {
            [[Logger sharedLogger] logAndStdOut:@"Block install cancelled"];

            self.progress = 0;
            self.currentState = STATE_CANCELED;
            helper_.stop();
            return;
        }

        int progressOfBlock = helper_.executeFilesStep();

        // block is finished?
        if (progressOfBlock >= 100)
        {
            [[Logger sharedLogger] logAndStdOut:@"Block install 100+"];

            self.progress = prevOverallProgress + (int)(100.0);
            callback_();
            break;
        }
        // error from block?
        else if (progressOfBlock < 0)
        {
            [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Block < 0: %i", progressOfBlock]];
            self.lastError = ERROR_OTHER;
            self.currentState = STATE_ERROR;
            callback_();
            helper_.stop();
            return;
        }
        else
        {
            // [[Logger sharedLogger] writeToLog:@"Block processing"];
            self.progress = prevOverallProgress + (int)(progressOfBlock);
            callback_();
        }
    }

    [[Logger sharedLogger] logAndStdOut:@"Done writing blocks"];

    // create symlink for cli
    [self runProcess:@"/bin/mkdir" args:@[@"-p", @"/usr/local/bin"]];
    [[Logger sharedLogger] logAndStdOut:@"Creating CLI symlink"];
    NSString *filepath = [NSString stringWithFormat:@"%@%@", [self getInstallPath], @"/Contents/MacOS/windscribe-cli"];
    NSString *sympath = @"/usr/local/bin/windscribe-cli";
    [[NSFileManager defaultManager] removeItemAtPath:sympath error:nil];
    [[NSFileManager defaultManager] createSymbolicLinkAtPath:sympath withDestinationPath:filepath error:nil];

    self.progress = 100;
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
    NSDate *waitingHelperSince_ = [NSDate date];
    while (!helper_.connect())
    {
        usleep(10000); // 10 milliseconds
        int seconds = -(int)[waitingHelperSince_ timeIntervalSinceNow];
        if (seconds > 5) {
            return NO;
        }
    }

    return YES;
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
