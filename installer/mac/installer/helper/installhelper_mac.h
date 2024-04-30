#pragma once

#import <Foundation/Foundation.h>

// functions for install helper
class InstallHelper_mac
{
public:
    static bool installHelper(bool bForceDeleteOld);

private:
    static bool isAppMajorMinorVersionSame();
};
