#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"
#include "ImageResources.h"
#include "ProgressView.h"
#include "ForegroundView.h"
#include "ToggleControl.h"

NS_ASSUME_NONNULL_BEGIN

@interface MainView : NSView
{
    ImageResources *imageResources_;
    bool bMouseButtonPressed_;
    NSPoint startDragPosition_;
    NSPoint startFrameOrigin_;
    ProgressView *progressView_;
    ForegroundView *fgView_;
    NSImageView *bgView_;

    CGFloat curBackgroundOpacity_;
    Boolean isIncOpacity_;
    NSTimer *opacityTimer_;
    
    BOOL isInstallScreen_;
    BOOL isInstalling_;
    BOOL isLaunching_;
};

@property (nonatomic, weak) IBOutlet AppDelegate *appDelegate;
@property (nonatomic, weak) IBOutlet NSButton *installButton;
@property (nonatomic, weak) IBOutlet NSButton *settingsButton;
@property (nonatomic, weak) IBOutlet NSButton *eulaButton;
@property (nonatomic, weak) IBOutlet NSButton *escButton;
@property (nonatomic, weak) IBOutlet ToggleControl *factoryResetToggle;
@property (nonatomic, weak) IBOutlet NSTextField *factoryResetField;

- (void)mouseDown:(NSEvent *)event;
- (void)mouseDragged:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)onOpacityTimer;

- (IBAction)onEulaClick:(id)sender;
- (IBAction)onInstallClick:(id)sender;
- (IBAction)onSettingsClick:(id)sender;
- (IBAction)onEscClick:(id)sender;
- (IBAction)onFactoryResetToggleClick:(id)sender;

- (void) installerCallback: (id) object;

-(void) updateState;

@end

NS_ASSUME_NONNULL_END
