#include "windowsizemanager.h"

#include <algorithm>

WindowSizeManager::WindowSizeManager()
{
}

void WindowSizeManager::addWindow(ResizableWindow *window, ShadowManager::SHAPE_ID shapeId, int resizeDurationMs)
{
    struct WindowInfo info;

    info.shapeId = shapeId;
    info.state = kWindowCollapsed;
    info.height = std::max(window->minimumHeight(), (int)window->boundingRect().height());
    info.prevHeight = info.height;
    info.resizeDurationMs = resizeDurationMs;
    info.scrollPos = 0;

    windowInfo_[window] = info;
}

std::vector<ResizableWindow *> WindowSizeManager::windows()
{
    std::vector<ResizableWindow *> keys;

    for (auto it = windowInfo_.begin(); it != windowInfo_.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

void WindowSizeManager::setWindowHeight(ResizableWindow *window, int height)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return;
    }
    if (height < window->minimumHeight()) {
        height = window->minimumHeight();
    }
    windowInfo_[window].height = height;
}

int WindowSizeManager::windowHeight(ResizableWindow *window)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return -1;
    }
    return windowInfo_[window].height;
}

void WindowSizeManager::setScrollPos(ResizableWindow *window, int pos)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return;
    }
    windowInfo_[window].scrollPos = pos;
}

int WindowSizeManager::scrollPos(ResizableWindow *window)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return -1;
    }
    return windowInfo_[window].scrollPos;
}

void WindowSizeManager::setPreviousWindowHeight(ResizableWindow *window, int height)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return;
    }
    windowInfo_[window].prevHeight = height;
}

int WindowSizeManager::previousWindowHeight(ResizableWindow *window)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return -1;
    }
    return windowInfo_[window].prevHeight;
}

void WindowSizeManager::setState(ResizableWindow *window, WindowState state)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return;
    }
    windowInfo_[window].state = state;
}

WindowSizeManager::WindowState WindowSizeManager::state(ResizableWindow *window)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return kWindowUnknown;
    }
    return windowInfo_[window].state;
}

bool WindowSizeManager::hasWindowInState(WindowState state)
{
    for (auto it = windowInfo_.begin(); it != windowInfo_.end(); ++it) {
        if (it->second.state == state) {
            return true;
        }
    }
    return false;
}

bool WindowSizeManager::allWindowsInState(WindowState state)
{
    for (auto it = windowInfo_.begin(); it != windowInfo_.end(); ++it) {
        if (it->second.state != state) {
            return false;
        }
    }
    return true;
}

bool WindowSizeManager::isExclusivelyExpanded(ResizableWindow *window)
{
    for (auto it = windowInfo_.begin(); it != windowInfo_.end(); ++it) {
        if ((it->first == window && it->second.state != kWindowExpanded) || (it->first != window && it->second.state == kWindowExpanded)) {
            return false;
        }
    }
    return true;
}

ShadowManager::SHAPE_ID WindowSizeManager::shapeId(ResizableWindow *window)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return (ShadowManager::SHAPE_ID_INVALID);
    }
    return windowInfo_[window].shapeId;
}

int WindowSizeManager::resizeDurationMs(ResizableWindow *window)
{
    if (windowInfo_.find(window) == windowInfo_.end()) {
        // not found
        return -1;
    }
    return windowInfo_[window].resizeDurationMs;
}