#import "base_installer.hpp"


@implementation BaseInstaller

@synthesize path = path_;

-(id)init
{
    if (self = [super init])
    {
        self.currentState = STATE_INIT;
        pathInd_ = 0;
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
        pathInd_ = 0;
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

-(void)start: (BOOL)keepBoth
{
}

-(void)cancel
{
}

-(NSString *)getFullInstallPath
{
    if (isUseUpdatePath_)
    {
        return updatePath_;
    }
    else
    {
        if (pathInd_ == 0)
        {
            return [self.path stringByAppendingString:@"/Windscribe.app"];
        }
        else
        {
            return [NSString stringWithFormat:@"%@/Windscribe %d.app",self.path, pathInd_];
        }
    }
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




