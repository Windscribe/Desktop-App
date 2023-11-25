 #pragma once

#import <Cocoa/Cocoa.h>

@interface Logger : NSObject
{
    NSString *aFile_;
    NSDate *startTime_;
}

+ sharedLogger;
- (void)writeToLog:(NSString *)str;
- (void)logAndStdOut:(NSString *)str;

@end
