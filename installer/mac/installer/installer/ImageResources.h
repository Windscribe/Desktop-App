#import <Foundation/Foundation.h>
#import <SVGKit/SVGKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface ImageResources : NSObject
{
    SVGKImage *badgeIcon_;
    SVGKImage *forwardArrowIcon_;
    SVGKImage *settingsIcon_;
    SVGKImage *checkIcon_;
    SVGKImage *folderIcon_;
    NSColor *backgroundColor_;
    NSImage *backgroundImage_;
}

@property (nonatomic, readonly) SVGKImage *badgeIcon;
@property (nonatomic, readonly) SVGKImage *forwardArrow;
@property (nonatomic, readonly) SVGKImage *settingsIcon;
@property (nonatomic, readonly) SVGKImage *checkIcon;
@property (nonatomic, readonly) SVGKImage *folderIcon;
@property (nonatomic, readonly) NSColor *backgroundColor;
@property (nonatomic, readonly) NSImage *backgroundImage;

@end

NS_ASSUME_NONNULL_END
