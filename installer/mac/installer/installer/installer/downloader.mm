#import "downloader.hpp"
#import "../Logger.h"

@implementation Downloader

- (instancetype)initWithUrl:(NSString *)url
{
    if (self = [super init])
    {
        strUrl_ = url;
        data_ = nil;
        session_ = nil;
    }
    return self;
}

- (void)setObjectForCallback: (SEL)aSelector withObject:(id)arg
{
    callbackSelector_ = aSelector;
    callbackObject_ = arg;
}

- (BOOL)isFolderAlreadyExist
{
    return NO;
}

-(void)start: (BOOL)keepBoth
{
    self.progress = 0;
    self.currentState = STATE_EXTRACTING;
    [callbackObject_ performSelectorOnMainThread:callbackSelector_ withObject:self waitUntilDone:NO];
    NSURLSessionConfiguration *conf = [NSURLSessionConfiguration defaultSessionConfiguration];
    session_ = [NSURLSession sessionWithConfiguration:conf  delegate: self delegateQueue: nil];
    
    [[session_ downloadTaskWithURL: [NSURL URLWithString:strUrl_]] resume];
}

// cancel and do uninstall
-(void)cancel
{
    self.currentState = STATE_CANCELED;
    if (session_)
    {
        [session_ invalidateAndCancel];
        session_ = nil;
    }
}

-(void)runLauncher
{
    std::string filePath = std::string([filePath_ UTF8String]);
    std::string cmd = "hdiutil attach '";
    cmd += filePath;
    cmd += "'";
     
    // run system command
    FILE *file = popen(cmd.c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
        }
        pclose(file);
    }
    
    // open DMG in Finder
    [[NSWorkspace sharedWorkspace] openURL: [NSURL fileURLWithPath:@"/Volumes/Windscribe" isDirectory:YES]];
    
    self.progress = 100;
    self.currentState = STATE_LAUNCHED;
    [callbackObject_ performSelectorOnMainThread:callbackSelector_ withObject:self waitUntilDone:NO];
}

- (void)URLSession:(NSURLSession *)session
      downloadTask:(NSURLSessionDownloadTask *)downloadTask
      didWriteData:(int64_t)bytesWritten
    totalBytesWritten:(int64_t)totalBytesWritten
    totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite
{
    NSHTTPURLResponse *httpResponce = (NSHTTPURLResponse *)downloadTask.response;
    NSInteger statusCode = httpResponce.statusCode;
    if (statusCode == 200)
    {
        double progress = (double)totalBytesWritten / (double)totalBytesExpectedToWrite * 100.0f;

        self.progress = progress;
        self.currentState = STATE_EXTRACTING;
        [callbackObject_ performSelectorOnMainThread:callbackSelector_ withObject:self waitUntilDone:NO];
    }
}

- (void)URLSession:(nonnull NSURLSession *)session downloadTask:(nonnull NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(nonnull NSURL *)location
{
    NSHTTPURLResponse *httpResponce = (NSHTTPURLResponse *)downloadTask.response;
    NSInteger statusCode = httpResponce.statusCode;
    if (statusCode == 200)
    {
        std::string filePath = getenv("TMPDIR");
        filePath += "windscribe_legacy_installer.dmg";
        filePath_ = [NSString stringWithUTF8String:filePath.c_str()];
        
        NSData *data = [NSData dataWithContentsOfURL:location];
        [data writeToFile:filePath_ atomically:YES];
        
        self.progress = 100;
        self.currentState = STATE_FINISHED;
        [callbackObject_ performSelectorOnMainThread:callbackSelector_ withObject:self waitUntilDone:NO];
    }
    else
    {
        self.progress = 0;
        self.currentState = STATE_ERROR;
        self.lastError = @"Can't download legacy installer. Please contact support.";
        [callbackObject_ performSelectorOnMainThread:callbackSelector_ withObject:self waitUntilDone:NO];
    }
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
              didCompleteWithError:(NSError *)error
{
    if (error)
    {
        self.progress = 0;
        self.currentState = STATE_ERROR;
        self.lastError = @"Can't download legacy installer. Please contact support.";
        [callbackObject_ performSelectorOnMainThread:callbackSelector_ withObject:self waitUntilDone:NO];
    }
}

@end




