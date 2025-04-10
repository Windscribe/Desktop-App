#pragma once

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include "base_installer.h"
#include "processes_helper.h"
#include "../helper/helper_installer.h"

NS_ASSUME_NONNULL_BEGIN

@interface Installer : BaseInstaller
{
    //std::list<IInstallBlock *> blocks_;
    dispatch_group_t d_group_;
    std::mutex mutex_;
    bool isCanceled_;

    std::unique_ptr<HelperInstaller> helper_;

    std::function<void()> callback_;
}

- (void)setCallback: (std::function<void()>)func;

- (BOOL)isFolderAlreadyExist;

- (void)start;
- (void)cancel;
- (void)runLauncher;
- (NSString *)runProcess: (NSString *)exePath args:(NSArray *)args;
- (BOOL)waitForProcessFinish: (std::vector<pid_t>)processes helper:(ProcessesHelper *)helper timeoutSec:(size_t)timeoutSec;
- (void)waitForCompletion;
- (BOOL)connectHelper;

@end

NS_ASSUME_NONNULL_END
