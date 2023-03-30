#ifndef Installer_hpp
#define Installer_hpp

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include "base_installer.hpp"
#include "processes_helper.h"
#include "../helper/helper_mac.h"

NS_ASSUME_NONNULL_BEGIN

@interface Installer : BaseInstaller
{
    //std::list<IInstallBlock *> blocks_;
    dispatch_group_t d_group_;
    std::mutex mutex_;
    bool isCanceled_;
    
    Helper_mac helper_;
    
    SEL callbackSelector_;
    id  callbackObject_;
}

- (void)setObjectForCallback: (SEL)aSelector withObject:(id)arg;

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

#endif /* Installer_hpp */
