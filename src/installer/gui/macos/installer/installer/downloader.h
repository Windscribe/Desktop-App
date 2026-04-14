#pragma once

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include "base_installer.h"

NS_ASSUME_NONNULL_BEGIN

@interface Downloader : BaseInstaller <NSURLSessionDelegate, NSURLSessionDownloadDelegate, NSURLSessionTaskDelegate>
{
    SEL callbackSelector_;
    id  callbackObject_;
    NSString *strUrl_;
    NSString *filePath_;
    NSMutableData *data_;
    NSURLSession *session_;
}

- (instancetype)initWithUrl:(NSString *)url;

- (void)setObjectForCallback: (SEL)aSelector withObject:(id)arg;

- (BOOL)isFolderAlreadyExist;

// keepBoth make sense if folder already exists
// keepBoth == YES -> create dir with added number
// keepBoth == NO -> overwrite old dir
- (void)start: (BOOL)keepBoth;

- (void)cancel;
- (void)runLauncher;

@end

NS_ASSUME_NONNULL_END
