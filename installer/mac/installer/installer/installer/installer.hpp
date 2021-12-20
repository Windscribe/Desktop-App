#ifndef Installer_hpp
#define Installer_hpp

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include "base_installer.hpp"
#include "helper_mac.h"

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
    BOOL isKeepBoth_;
}

- (void)setObjectForCallback: (SEL)aSelector withObject:(id)arg;

- (BOOL)isFolderAlreadyExist;

// keepBoth make sense if folder already exists
// keepBoth == YES -> create dir with added number
// keepBoth == NO -> overwrite old dir
- (void)start: (BOOL)keepBoth;


- (void)cancel;
- (void)runLauncher;
- (void)waitForCompletion;

@end

NS_ASSUME_NONNULL_END

#endif /* Installer_hpp */
