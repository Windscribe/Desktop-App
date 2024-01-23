#import <Foundation/Foundation.h>
#import "Logger.h"

@implementation Logger

+ sharedLogger {
    static id sharedTask = nil;
    if(sharedTask==nil) {
        sharedTask = [[Logger alloc] init];
    }
    return sharedTask;
}

-(id)init
{
    startTime_ = [NSDate date];

    NSString *logFolderOuter =[NSString stringWithFormat:@"%@/Library/Application Support/Windscribe", NSHomeDirectory()];
    NSString *logFolder = [NSString stringWithFormat:@"%@/Windscribe", logFolderOuter];
    aFile_ = [NSString stringWithFormat:@"%@/log_installer.txt", logFolder];

    // Create logFolder (deletes name-colliding files if necessary)
    BOOL isInnerDir;
    BOOL innerExists = [[NSFileManager defaultManager] fileExistsAtPath:logFolder isDirectory:&isInnerDir];
    if (innerExists)
    {
        if (!isInnerDir)
        {
            [[NSFileManager defaultManager] removeItemAtPath:logFolder error:nil];
            [[NSFileManager defaultManager] createDirectoryAtPath:logFolder withIntermediateDirectories:YES attributes:nil error:nil];
        }
    }
    else
    {
        [[NSFileManager defaultManager] removeItemAtPath:logFolderOuter error:nil];
        [[NSFileManager defaultManager] createDirectoryAtPath:logFolder withIntermediateDirectories:YES attributes:nil error:nil];
    }

    // move previous log to be prev_log_installer.txt
    if ([[NSFileManager defaultManager] fileExistsAtPath:aFile_])
    {
        NSString *prevLogFile = [NSString stringWithFormat:@"%@/prev_log_installer.txt", logFolder];
        [[NSFileManager defaultManager] removeItemAtPath:prevLogFile error:nil];
        [[NSFileManager defaultManager] moveItemAtPath:aFile_ toPath:prevLogFile error:nil];
    }

    // create new log
    [[NSFileManager defaultManager] createFileAtPath:aFile_ contents:nil attributes:nil];
    [self logAndStdOut:@"Installer log created"];
    return self;
}

- (void)writeToLog:(NSString *)str
{
    @autoreleasepool {
        NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
        NSTimeZone *timeZone = [NSTimeZone timeZoneWithName:@"UTC"];
        [dateFormatter setTimeZone:timeZone];
        [dateFormatter setDateFormat:@"ddMMYY HH:mm:ss:SSS"];
        NSDate *currentTime = [NSDate date];

        NSFileHandle *aFileHandle = [NSFileHandle fileHandleForWritingAtPath:aFile_];
        [aFileHandle seekToEndOfFile];
        [aFileHandle writeData:[[NSString stringWithFormat:@"[%@ %10.03f] %@\n",
                                 [dateFormatter stringFromDate:currentTime], [currentTime timeIntervalSinceDate:startTime_], str]
                                dataUsingEncoding:NSASCIIStringEncoding]];
        [aFileHandle closeFile];
    }
}

-(void)logAndStdOut:(NSString *)str
{
    NSLog(@"%@", str);
    [self writeToLog:str];
}

@end
