#import "ForegroundView.h"
#import "Logger.h"
#import <SVGKit/SVGKit.h>

@implementation ForegroundView

@synthesize imageResources = imageResources_;
@synthesize caption = caption_;
@synthesize isInstallScreen = isInstallScreen_;
@synthesize installButtonRect = installButtonRect_;
@synthesize bgOpacity = bgOpacity_;

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    [[NSGraphicsContext currentContext] saveGraphicsState];
    
    NSColor *fgColor = [NSColor colorWithCalibratedRed: 1.0f green: 1.0f blue: 1.0f alpha: 1.0f];
    [fgColor set];
    
    NSRect clientRect = [self bounds];
    NSSize imgSize = imageResources_.backgroundImage.size;
    NSRect rc = CGRectMake((clientRect.size.width - imgSize.width) / 2, (clientRect.size.height - imgSize.height) / 2, imgSize.width, imgSize.height);
    
    // draw logo icon
    imgSize = imageResources_.badgeIcon.NSImage.size;
    CGFloat xLogo = (clientRect.size.width - imgSize.width) / 2;
    CGFloat yLogo = clientRect.size.height - 16 - imgSize.height;

    // 1) fill with black circle
    /*
    CGFloat margin = 3;
    [imageResources_.backgroundColor setStroke];
    [imageResources_.backgroundColor setFill];
    NSRect rect = NSMakeRect(xLogo + margin, yLogo + margin, imgSize.width - margin * 2, imgSize.height - margin * 2);
    NSBezierPath* circlePath = [NSBezierPath bezierPath];
    [circlePath appendBezierPathWithOvalInRect: rect];
    [circlePath stroke];
    [circlePath fill];
     */

    // 2) logo icon
    rc = CGRectMake(xLogo, yLogo, imgSize.width, imgSize.height);
    [imageResources_.badgeIcon.NSImage drawInRect:rc];
            
    // draw dividers on settings screen only
    if (isInstallScreen_ == NO)
    {
        NSColor *dividerColor = [NSColor colorWithCalibratedRed: 1.0f green: 1.0f blue: 1.0f alpha: 0.275f];
        [dividerColor set];
        NSRect dividerRect = NSMakeRect(28, 135, clientRect.size.width - 28, 2);
        NSBezierPath *rectPath = [NSBezierPath bezierPathWithRect:dividerRect];
        [rectPath fill];
        dividerRect = NSMakeRect(28, 85, clientRect.size.width - 28, 2);
        rectPath = [NSBezierPath bezierPathWithRect:dividerRect];
        [rectPath fill];
    }
    
    [self drawCaption];
    
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (void)drawCaption
{
    NSString *strCaption = caption_;
    
     NSColor *colorDark = [NSColor colorWithCalibratedRed: 0.0 green: 0.0 blue: 0.0 alpha: 1];
     NSColor *colorWhite = UIColor.whiteColor;
    // draw caption with shadow
    NSMutableParagraphStyle* textStyle = NSMutableParagraphStyle.defaultParagraphStyle.mutableCopy;
    textStyle.alignment = NSTextAlignmentLeft;

    NSRect btnRect = installButtonRect_;
    
    NSMutableDictionary *textFontAttributesDark = [NSMutableDictionary dictionary];
    [textFontAttributesDark setObject:colorDark forKey:NSForegroundColorAttributeName];
    [textFontAttributesDark setObject:[NSFont fontWithName:@"IBM Plex Sans Bold" size:17] forKey:NSFontAttributeName];
    
    NSMutableDictionary *textFontAttributesWhite = [NSMutableDictionary dictionary];
    [textFontAttributesWhite setObject:colorWhite forKey:NSForegroundColorAttributeName];
    [textFontAttributesWhite setObject:[NSFont fontWithName:@"IBM Plex Sans Bold" size:17] forKey:NSFontAttributeName];
        
    NSSize size = [strCaption sizeWithAttributes:textFontAttributesWhite];
    NSRect rc = [self bounds];
    NSPoint pt = NSMakePoint((rc.size.width - size.width) / 2.0f, btnRect.origin.y + size.height + 22);
    NSPoint ptForDark = NSMakePoint(pt.x + 1.0f, pt.y - 1.0f);

    [strCaption drawAtPoint: ptForDark withAttributes: textFontAttributesDark];
    [strCaption drawAtPoint: pt withAttributes: textFontAttributesWhite];
}

@end
