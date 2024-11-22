#pragma once
#import <Foundation/Foundation.h>
#include <string>

inline std::string toStdString(NSString *str)
{
    std::string res;
    if (str)
        res = [str UTF8String];
    return res;
}

inline std::string toStdString(CFStringRef str)
{
    std::string res;
    if (str)
        res = [(__bridge NSString *)str UTF8String];
    return res;
}
