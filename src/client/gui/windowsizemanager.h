#pragma once

#include <map>
#include "commongraphics/resizablewindow.h"
#include "utils/shadowmanager.h"

class WindowSizeManager
{
public:
    enum WindowState { kWindowUnknown, kWindowExpanded, kWindowCollapsed, kWindowAnimating };

    WindowSizeManager();

    void addWindow(ResizableWindow *window, ShadowManager::SHAPE_ID shape_id, int resizeDurationMs);
    std::vector<ResizableWindow *> windows();

    void setWindowHeight(ResizableWindow *window, int height);
    int windowHeight(ResizableWindow *window);

    void setScrollPos(ResizableWindow *window, int pos);
    int scrollPos(ResizableWindow *window);

    void setPreviousWindowHeight(ResizableWindow *window, int height);
    int previousWindowHeight(ResizableWindow *window);
    bool hasWindowInState(WindowState state);
    bool allWindowsInState(WindowState state);
    bool isExclusivelyExpanded(ResizableWindow *window);

    void setState(ResizableWindow *window, WindowState state);
    WindowState state(ResizableWindow *window);

    ShadowManager::SHAPE_ID shapeId(ResizableWindow *window);

    int resizeDurationMs(ResizableWindow *window);

private:
    struct WindowInfo {
        ShadowManager::SHAPE_ID shapeId;
        WindowState state;
        int height;
        int prevHeight;
        int resizeDurationMs;
        int scrollPos;
    };
    std::map<ResizableWindow *, struct WindowInfo> windowInfo_;
};