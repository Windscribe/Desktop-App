#import <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

#include "interfaceutils.h"
#include "utils/macutils.h"

namespace InterfaceUtils {

bool isDarkMode()
{
    bool isDarkModeSet = false;
    if (@available(macOS 10.14, *)) {
        NSAppearanceName appearanceName =
            [[[NSApplication sharedApplication] effectiveAppearance] name];
        if (appearanceName && appearanceName == NSAppearanceNameDarkAqua)
            isDarkModeSet = true;
    }
    return isDarkModeSet;
}

}
