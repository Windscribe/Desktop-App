
#import "MainWindow.h"

extern int g_window_center_x;
extern int g_window_center_y;

@implementation MainWindow

-(void)awakeFromNib {
    [[self standardWindowButton:NSWindowZoomButton] setHidden:YES];
    
    if (g_window_center_x != INT_MAX && g_window_center_y != INT_MAX)
    {
        NSRect rc = [self frame];
        [self setFrameOrigin:NSMakePoint(g_window_center_x - rc.size.width / 2, g_window_center_y - rc.size.height / 2)];
    }
}
@end
