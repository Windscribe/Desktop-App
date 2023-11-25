#import <mach/machine.h>
#import <sys/types.h>
#import <sys/sysctl.h>

#include <QApplication>

#import "Logger.h"
#import "mainwindow.h"
#import "../../../client/common/version/windscribe_version.h"

namespace
{

bool CheckCommandLineArgument(int argc, char *argv[], const char *argumentToCheck, int *valueIndex = nullptr,
                              int *valueCount = nullptr)
{
    if (!argumentToCheck) {
        return false;
    }

    bool result = false;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], argumentToCheck)) {
            continue;
        }
        result = true;
        if (!valueCount) {
            continue;
        }
        *valueCount = 0;
        if (valueIndex) {
            *valueIndex = -1;
        }
        for (int j = i + 1; j < argc; ++j) {
            if (strncmp(argv[j], "-", 1)) {
                if (valueIndex && *valueIndex < 0) {
                    *valueIndex = j;
                }
                ++*valueCount;
            } else {
                break;
            }
        }
    }
    return result;
}

bool hasEnding (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool removeLastPartOfPath(std::string &path)
{
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos)
    {
        path.erase(pos);
    }
    return true;
}

void logSystemInfo()
{
    NSMutableString *data = [[NSMutableString alloc] init];
    cpu_type_t type;
    size_t size;
    cpu_subtype_t subtype;
    int ret1, ret2;

    size = sizeof(type);
    ret1 = sysctlbyname("hw.cputype", &type, &size, NULL, 0);

    size = sizeof(subtype);
    ret2 = sysctlbyname("hw.cpusubtype", &subtype, &size, NULL, 0);

    if (ret1 != 0) {
        [data appendString: @"Unknown"];
    } else if (type == CPU_TYPE_X86 || type == CPU_TYPE_X86_64) {
        if (ret2 != 0) {
            [data appendString: @"x86 (unknown subtype)"];
        } else if (subtype == CPU_SUBTYPE_X86_64_ALL || subtype == CPU_SUBTYPE_X86_64_H) {
            [data appendString: @"x86_64"];
        } else if (subtype == CPU_SUBTYPE_X86_ALL || subtype == CPU_SUBTYPE_X86_ARCH1) {
            [data appendString: @"x86"];
        } else {
            [data appendString: [NSString stringWithFormat:@"x86 (unknown subtype %d)", subtype]];
        }
    } else if (type == CPU_TYPE_ARM) {
        [data appendString: @"arm"];
    } else if (type == CPU_TYPE_ARM64) {
        [data appendString: @"arm64)"];
    } else {
        [data appendString: [NSString stringWithFormat:@"Unrecognized (%d)", type]];
    }

    NSProcessInfo *processInfo = [NSProcessInfo processInfo];

    [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"App version: %s", WINDSCRIBE_VERSION_STR]];
    [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"CPU architecture: %@", data]];
    [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"MacOS version: %@", processInfo.operatingSystemVersionString]];
}

void updatePathToAppFolder(std::string &path)
{
    std::string lowerCase = path;
    std::string ending_old_versions = "contents/library/windscribeengine.app/contents/macos";
    std::string ending_new_versions = "contents/macos";
    std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    int cnt_delete = 0;
    // for support old versions, when there was a division into modules the engine and gui
    if (hasEnding(lowerCase, ending_old_versions))
    {
        // example path: "/Applications/Windscribe.app/Contents/Library/WindscribeEngine.app/Contents/MacOS"
        // remove 5 parts of path from ending
        cnt_delete = 5;
    }
    // for new versions of app
    else if (hasEnding(lowerCase, ending_new_versions))
    {
        // example path: "/Application/Windscribe.app/Contents/MacOS"
        // remove 2 parts of path from ending
        cnt_delete = 2;
    }

    // remove cnt_delete parts of path from ending
    for (int i = 0; i < cnt_delete; ++i)
    {
        if (!removeLastPartOfPath(path))
        {
            break;
        }
    }
}

}  // namespace


int main(int argc, char *argv[]) {
    InstallerOptions ops;

    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
    }

    ops.updating = CheckCommandLineArgument(argc, argv, "-update");
    int expectedArgumentCount = ops.updating ? 2 : 1;

    int center_coord_index = 0, center_coord_count = 0;
    if (CheckCommandLineArgument(argc, argv, "-center", &center_coord_index, &center_coord_count)) {
        if (center_coord_count > 0)
            ops.centerX = atoi(argv[center_coord_index]);
        if (center_coord_count > 1)
            ops.centerY = atoi(argv[center_coord_index+1]);
        expectedArgumentCount += 1 + center_coord_count;
    }

    logSystemInfo();

    QApplication a(argc, argv);
    MainWindow w(true, ops);
    w.show();

    return a.exec();
}
