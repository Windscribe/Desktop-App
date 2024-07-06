#pragma once

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include "../helper/helper_mac.h"

NS_ASSUME_NONNULL_BEGIN

enum INSTALLER_CURRENT_STATE { STATE_INIT, STATE_EXTRACTING, STATE_CANCELED, STATE_FINISHED, STATE_ERROR, STATE_LAUNCHED, STATE_EXTRACTED };
enum INSTALLER_ERROR { ERROR_OTHER = 1, ERROR_PERMISSION, ERROR_KILL, ERROR_CONNECT_HELPER, ERROR_DELETE, ERROR_UNINSTALL, ERROR_MOVE_CUSTOM_DIR };

@interface BaseInstaller : NSObject
{
    bool isUseUpdatePath_;
    NSString *updatePath_;
}

@property(atomic, assign) enum INSTALLER_CURRENT_STATE currentState;
@property(atomic, assign) enum INSTALLER_ERROR lastError;
@property(atomic, assign) int progress;
@property(atomic, assign) bool factoryReset;
@property(atomic, strong) NSString *path;

- (id)initWithUpdatePath:(NSString *)path;

- (void)setObjectForCallback: (SEL)aSelector withObject:(id)arg;

- (BOOL)isFolderAlreadyExist;

- (NSString *)getOldInstallPath;
- (NSString *)getInstallPath;

- (void)start;
- (void)cancel;
- (void)runLauncher;
- (void)waitForCompletion;

@end

NS_ASSUME_NONNULL_END
