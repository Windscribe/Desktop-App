#ifndef QuietModeApplication_h
#define QuietModeApplication_h

#import "AppDelegate.h"

@interface QuietModeHelper : NSObject
{
}

@property (nonatomic, strong) IBOutlet AppDelegate *appDelegate;

- (void) start;
- (id) initWithPath:(NSString *)path andDelegate:(AppDelegate *)appDelegate;

- (void) installerCallback: (id) object;
- (void)appDidLaunch:(NSNotification*)note;

@end

#endif /* QuietModeApplication_h */
