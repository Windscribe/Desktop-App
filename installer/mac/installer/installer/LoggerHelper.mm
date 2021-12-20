#include "LoggerHelper.h"
#import "Logger.h"

LoggerHelper::LoggerHelper() 
{
    
}

void LoggerHelper::writeToLog(const char *str)
{
    [[Logger sharedLogger] writeToLog:[NSString stringWithUTF8String:str]];
}

void LoggerHelper::logAndStdOut(const char *str)
{
    NSLog(@"%@", [NSString stringWithUTF8String:str]);
    writeToLog(str);
}
