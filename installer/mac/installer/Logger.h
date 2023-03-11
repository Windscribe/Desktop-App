 #ifndef LOGGER_H
 #define LOGGER_H

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


#endif // LOGGER_H

