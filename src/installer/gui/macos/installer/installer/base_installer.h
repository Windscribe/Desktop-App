#pragma once

#import <Foundation/Foundation.h>

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

- (NSString *)getOldInstallPath;
- (NSString *)getInstallPath;

@end

NS_ASSUME_NONNULL_END
