#import "AppDelegate.h"
#import "installer/installer.hpp"
#import "installer/downloader.hpp"

extern bool g_isUpdateMode;
extern std::string g_path;

@implementation AppDelegate

@synthesize installer = installer_;

-(id)init
{
    self = [super init];
    if (self)
    {
        if (floor(NSAppKitVersionNumber) < NSAppKitVersionNumber10_11)  // MacOS 10.9-10.10
        {
            installer_ = [[Downloader alloc] initWithUrl:@"https://assets.totallyacdn.com/desktop/mac/Windscribe_1.80.dmg"];
            self.isLegacy = true;
        }
        else if (floor(NSAppKitVersionNumber) < NSAppKitVersionNumber10_12) // MacOS 10.11
        {
            installer_ = [[Downloader alloc] initWithUrl:@"https://assets.totallyacdn.com/desktop/mac/Windscribe_1.83.dmg"];
            self.isLegacy = true;
            
        }
        else
        {
            if (g_path.empty())
                installer_ = [[Installer alloc] init];
            else
                installer_ = [[Installer alloc] initWithUpdatePath: [NSString stringWithCString:g_path.c_str()
                encoding:[NSString defaultCStringEncoding]] ];
            self.isLegacy = false;
        }
        self.isUpdateMode = g_isUpdateMode;
    }
    return self;
}
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    if (installer_.currentState == STATE_EXTRACTING)
    {
        [installer_ cancel];
    }
}

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *) sender
{
    return YES;
}



@end
