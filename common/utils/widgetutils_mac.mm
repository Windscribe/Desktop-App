#import <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

#include <QIcon>
#include "widgetutils_mac.h"
#include "macutils.h"
#include "dpiscalemanager.h"

#include <QDebug>

QPixmap *WidgetUtils_mac::extractProgramIcon(const QString &filePath)
{
    // TODO: QIcon constructor sometimes throws this WARNING into log:
    // "libpng warning: iCCP: known incorrect sRGB profile"
    QIcon icon(filePath);
    int size = 16 * G_SCALE;
    return new QPixmap(icon.pixmap(QSize(size, size)));
}

void WidgetUtils_mac::allowMinimizeForFramelessWindow(QWidget *window)
{
#if defined __APPLE__ && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
    if(@available(macOS 10.13, *))
    {
        NSWindow* nsWindow = [(NSView*)(window->winId()) window];
        [nsWindow setStyleMask:(/*NSWindowStyleMaskResizable |*/ NSWindowStyleMaskBorderless | NSWindowStyleMaskFullSizeContentView | NSWindowStyleMaskMiniaturizable)];
        [nsWindow setTitlebarAppearsTransparent:YES];       // 10.10+
        [nsWindow setTitleVisibility:NSWindowTitleHidden];  // 10.10+
        [nsWindow setShowsToolbarButton:NO];
        [nsWindow setBackgroundColor:[NSColor clearColor]];

        [[nsWindow standardWindowButton:NSWindowFullScreenButton] setHidden:YES];
        [[nsWindow standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
        [[nsWindow standardWindowButton:NSWindowCloseButton] setHidden:YES];
        [[nsWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];
    }
#else
    Q_UNUSED(window);
#endif
}
