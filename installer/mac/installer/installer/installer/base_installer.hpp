#ifndef BaseInstaller_hpp
#define BaseInstaller_hpp

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include "helper_mac.h"

NS_ASSUME_NONNULL_BEGIN

enum INSTALLER_CURRENT_STATE { STATE_INIT, STATE_EXTRACTING, STATE_CANCELED, STATE_FINISHED, STATE_ERROR, STATE_LAUNCHED };

@interface BaseInstaller : NSObject
{
    int pathInd_;  // 0 - no prefix added to path, >=1 - number prefix added to path
    bool isUseUpdatePath_;
    NSString *updatePath_;
}

@property(atomic, assign) enum INSTALLER_CURRENT_STATE currentState;
@property(atomic, assign) int progress;
@property(atomic, assign) bool factoryReset;
@property(atomic, strong) NSString *path;
@property(atomic, strong) NSString *lastError;

- (id)initWithUpdatePath:(NSString *)path;

- (void)setObjectForCallback: (SEL)aSelector withObject:(id)arg;

- (BOOL)isFolderAlreadyExist;

-(NSString *)getFullInstallPath;

// keepBoth make sense if folder already exists
// keepBoth == YES -> create dir with added number
// keepBoth == NO -> overwrite old dir
- (void)start: (BOOL)keepBoth;


- (void)cancel;
- (void)runLauncher;
- (void)waitForCompletion;

@end

NS_ASSUME_NONNULL_END

#endif /* BaseInstaller_hpp */
