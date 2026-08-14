#import <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

#include <QIcon>
#include "widgetutils_mac.h"
#include "dpiscalemanager.h"

#include <QDebug>

void WidgetUtils_mac::allowMinimizeForFramelessWindow(QWidget *window)
{
    NSWindow* nsWindow = [(__bridge NSView*)reinterpret_cast<void*>(window->winId()) window];
    [nsWindow setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorTransient];

#if defined __APPLE__ && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
    if(@available(macOS 10.13, *))
    {
        [nsWindow setStyleMask:(/*NSWindowStyleMaskResizable |*/ NSWindowStyleMaskBorderless | NSWindowStyleMaskFullSizeContentView | NSWindowStyleMaskMiniaturizable)];
        [nsWindow setTitlebarAppearsTransparent:YES];       // 10.10+
        [nsWindow setTitleVisibility:NSWindowTitleHidden];  // 10.10+
        [nsWindow setShowsToolbarButton:NO];
        [nsWindow setBackgroundColor:[NSColor clearColor]];

        [[nsWindow standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
        [[nsWindow standardWindowButton:NSWindowCloseButton] setHidden:YES];
        [[nsWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];
    }
#else
    Q_UNUSED(window);
#endif
}

void WidgetUtils_mac::allowMoveBetweenSpacesForWindow(QWidget *window, bool docked, bool moveWindow)
{
    NSWindow* nsWindow = [(__bridge NSView*)reinterpret_cast<void*>(window->winId()) window];
    if (docked) {
        [nsWindow setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorTransient];
    } else if (moveWindow) {
        [nsWindow setCollectionBehavior:NSWindowCollectionBehaviorMoveToActiveSpace];
    } else {
        [nsWindow setCollectionBehavior:NSWindowCollectionBehaviorDefault];
    }
}

void WidgetUtils_mac::setNeedsDisplayForWindow(QWidget *widget)
{
    NSWindow *nsWindow = [(__bridge NSView*)reinterpret_cast<void*>(widget->winId()) window];
    [[nsWindow contentView] setNeedsDisplay:YES];
}
