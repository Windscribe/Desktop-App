#import "base_installer.h"


@implementation BaseInstaller

@synthesize path = path_;

-(id)init
{
    if (self = [super init])
    {
        self.currentState = STATE_INIT;
        self.path = [self getDefaultInstallPath];
        isUseUpdatePath_ = false;
    }
    return self;
}

-(id)initWithUpdatePath: (NSString *)path
{
    if (self = [super init])
    {
        self.currentState = STATE_INIT;
        isUseUpdatePath_ = true;
        updatePath_ = path;
        self.path = [self getDefaultInstallPath];

    }
    return self;
}

- (void)setObjectForCallback: (SEL)aSelector withObject:(id)arg
{
}

- (BOOL)isFolderAlreadyExist
{
    return NO;
}

-(void)start
{
}

-(void)cancel
{
}

-(NSString *)getOldInstallPath
{
    if (isUseUpdatePath_) {
        return updatePath_;
    } else {
        return [self.path stringByAppendingString:@"/Windscribe.app"];
    }
}

-(NSString *)getInstallPath
{
    return [self.path stringByAppendingString:@"/Windscribe.app"];
}

-(void)runLauncher
{
}

-(void)waitForCompletion
{

}

-(NSString *)getDefaultInstallPath
{
    NSFileManager *manager = [NSFileManager defaultManager];
    NSError *error;
    NSURL *applicationSupport = [manager URLForDirectory:NSApplicationDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:false error:&error];

    return applicationSupport.path;
}

@end




