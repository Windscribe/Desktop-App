#import <Cocoa/Cocoa.h>
#import "ImageResources.h"

IB_DESIGNABLE
@interface ToggleControl: NSControl

@property (nonatomic, assign) IBInspectable BOOL checked;

- (void)setImageResources:(ImageResources *)resources;

@end
