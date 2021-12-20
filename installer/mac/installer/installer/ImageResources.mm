#import "ImageResources.h"

@implementation ImageResources

@synthesize badgeIcon = badgeIcon_;
@synthesize forwardArrow = forwardArrowIcon_;
@synthesize backgroundColor = backgroundColor_;
@synthesize settingsIcon = settingsIcon_;
@synthesize checkIcon = checkIcon_;
@synthesize folderIcon = folderIcon_;
@synthesize backgroundImage = backgroundImage_;

-(id)init
{
    self = [super init];
    backgroundImage_ = [NSImage imageNamed:@"background"];
    backgroundColor_ = [NSColor colorWithCalibratedRed:3.0f/255.0f green:9.0f/255.0f blue:28.0f/255.0f alpha:1.0f];
        
    badgeIcon_ = [SVGKImage imageNamed: @"BADGE_ICON.svg"];
    forwardArrowIcon_ = [SVGKImage imageNamed: @"FRWRD_ARROW_ICON.svg"];
    settingsIcon_ = [SVGKImage imageNamed: @"SETTINGS_ICON.svg"];
    checkIcon_ = [SVGKImage imageNamed: @"CHECK_ICON.svg"];
    folderIcon_ = [SVGKImage imageNamed: @"FOLDER_ICON.svg"];
    
    return self;
}

@end
