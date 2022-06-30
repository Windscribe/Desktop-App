#import <Cocoa/Cocoa.h>

#import "QuietModeHelper.h"
#import "AppDelegate.h"
#import "Logger.h"

bool g_isUpdateMode = false;
int g_window_center_x = INT_MAX;
int g_window_center_y = INT_MAX;
std::string g_path;

namespace
{

bool CheckCommandLineArgument(int argc, const char * argv[], const char * argumentToCheck, int *valueIndex = nullptr,
                              int *valueCount = nullptr)
{
    if (!argumentToCheck)
        return false;

    bool result = false;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], argumentToCheck))
            continue;
        result = true;
        if (!valueCount)
            continue;
        *valueCount = 0;
        if (valueIndex)
            *valueIndex = -1;
        for (int j = i + 1; j < argc; ++j) {
            if (strncmp(argv[j], "-", 1)) {
                if (valueIndex && *valueIndex < 0)
                    *valueIndex = j;
                ++*valueCount;
            }
            else
            {
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


int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
    }
    
    g_isUpdateMode = CheckCommandLineArgument(argc, argv, "-update");
    int expectedArgumentCount = g_isUpdateMode ? 2 : 1;
    
    int center_coord_index = 0, center_coord_count = 0;
    if (CheckCommandLineArgument(argc, argv, "-center", &center_coord_index, &center_coord_count)) {
        if (center_coord_count > 0)
            g_window_center_x = atoi(argv[center_coord_index]);
        if (center_coord_count > 1)
            g_window_center_y = atoi(argv[center_coord_index+1]);
        expectedArgumentCount += 1 + center_coord_count;
    }
    
    int path_index;
    int path_count;
    if (CheckCommandLineArgument(argc, argv, "-path", &path_index, &path_count)) {
        if (path_count > 0)
            g_path = argv[path_index];
        expectedArgumentCount += 1 + path_count;
    }
    // for old compatibility
    else if (CheckCommandLineArgument(argc, argv, "-q", &path_index, &path_count)) {
        g_isUpdateMode = true;
        if (path_count > 0)
            g_path = argv[path_index];
        expectedArgumentCount += 1 + path_count;
    }
    
    updatePathToAppFolder(g_path);
    
    return NSApplicationMain(argc, argv);
}
