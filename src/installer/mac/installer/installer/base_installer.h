#pragma once

#import <Foundation/Foundation.h>

#include <list>
#include <stdio.h>
#include <string>
#include <thread>

#include "../helper/helper_mac.h"
#include "installerenums.h"

NS_ASSUME_NONNULL_BEGIN

@interface BaseInstaller : NSObject
{
    bool isUseUpdatePath_;
    NSString *updatePath_;
}

@property(atomic, assign) enum wsl::INSTALLER_STATE currentState;
@property(atomic, assign) enum wsl::INSTALLER_ERROR lastError;
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
