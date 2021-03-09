#include <Windows.h>
#include "gui/application.h"

namespace
{
int argCount = 0;
LPWSTR *argList = nullptr;

bool CheckCommandLineArgument(LPCWSTR argumentToCheck, int *valueIndex = nullptr,
                              int *valueCount = nullptr)
{
    if (!argumentToCheck)
        return false;

    if (!argList)
        argList = CommandLineToArgvW(GetCommandLine(), &argCount);

    bool result = false;
    for (int i = 0; i < argCount; ++i) {
        if (wcscmp(argList[i], argumentToCheck))
            continue;
        result = true;
        if (!valueCount)
            continue;
        *valueCount = 0;
        if (valueIndex)
            *valueIndex = -1;
        for (int j = i + 1; j < argCount; ++j) {
            if (wcsncmp(argList[j], L"-", 1)) {
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
}  // namespace


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    const bool isUpdateMode =
        CheckCommandLineArgument(L"-update") || CheckCommandLineArgument(L"-q");
    int expectedArgumentCount = isUpdateMode ? 2 : 1;

    int window_center_x = -1, window_center_y = -1;
    int center_coord_index = 0, center_coord_count = 0;
    if (CheckCommandLineArgument(L"-center", &center_coord_index, &center_coord_count)) {
        if (center_coord_count > 0)
            window_center_x = _wtoi(argList[center_coord_index]);
        if (center_coord_count > 1)
            window_center_y = _wtoi(argList[center_coord_index+1]);
        expectedArgumentCount += 1 + center_coord_count;
    }
    if (argCount != expectedArgumentCount) {
        printf("Incorrect number of arguments passed to installer.\n");
        return 0;
    }

    Application app(hInstance, nCmdShow, isUpdateMode);
    if (app.init(window_center_x, window_center_y))
        return app.exec();
    return -1;
}
