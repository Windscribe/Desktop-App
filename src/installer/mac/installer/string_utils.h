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