#include "exithandler_mac.h"
#include "utils/log/categories.h"
#import <Cocoa/Cocoa.h>
#include <objc/objc.h>
#include <objc/message.h>

IMP g_prevCloseImpl = NULL;
ExitHandler_mac *ExitHandler_mac::this_ = NULL;

// replace method for applicationShouldTerminate ( default Qt logic not acceptable for our needs)
NSApplicationTerminateReply exitMacHandler(id self, SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
    NSAppleEventManager *m = [NSAppleEventManager sharedAppleEventManager];
    NSAppleEventDescriptor *desc = [m currentAppleEvent];

    if (ExitHandler_mac::this_ && desc != nil) {
        NSAppleEventDescriptor *descQuitReason = [desc attributeDescriptorForKeyword:kAEQuitReason];
        if (descQuitReason != nil) {
            OSType eventType = [descQuitReason enumCodeValue];
            if (eventType == kAELogOut || eventType == kAEReallyLogOut) {
                qCInfo(LOG_BASIC) << "app close, reason: log out";
                ExitHandler_mac::this_->setExitWithRestart();
            } else if (eventType == kAEShowRestartDialog || eventType == kAERestart) {
                qCInfo(LOG_BASIC) << "app close, reason: system restart";
                ExitHandler_mac::this_->setExitWithRestart();
            } else if (eventType == kAEShowShutdownDialog || eventType == kAEShutDown) {
                qCInfo(LOG_BASIC) << "app close, reason: system shutdown";
                ExitHandler_mac::this_->setExitWithRestart();
            }
        }
    }

    if (ExitHandler_mac::this_) {
        emit ExitHandler_mac::this_->shouldTerminate();
    }

    if (g_prevCloseImpl) {
        NSApplicationTerminateReply ret = ( (NSApplicationTerminateReply (*)(id, SEL)) (*g_prevCloseImpl))( self, sel_registerName("applicationShouldTerminate:"));
        return ret;
    } else {
        return NSTerminateNow;
    }
    return NSTerminateCancel;
}

ExitHandler_mac::ExitHandler_mac(QObject *parent) : QObject(parent), bExitWithRestart_(false)
{
    this_ = this;

    id (*performMsgSend)(id, SEL) = (id (*)(id, SEL)) objc_msgSend; // define parametered version of objc_msgSend to mimic previous runtime version's parameters

    Class cls = objc_getClass("NSApplication");
    id appInst = performMsgSend((id)cls, sel_registerName("sharedApplication"));

    if (appInst != NULL) {
        id delegate = performMsgSend(appInst, sel_registerName("delegate"));
        Class delClass = (Class)performMsgSend(delegate,  sel_registerName("class"));
        SEL exitHandle = sel_registerName("applicationShouldTerminate:");

        Method mtdFunc = class_getInstanceMethod(delClass, exitHandle);
        if (mtdFunc == NULL) {
            return;
        }
        const char *types = method_getTypeEncoding(mtdFunc);

        if (class_getInstanceMethod(delClass, exitHandle)) {
            g_prevCloseImpl = class_replaceMethod(delClass, exitHandle, (IMP)exitMacHandler, types);
        } else {
            class_addMethod(delClass, exitHandle, (IMP)exitMacHandler, types);
        }
    }
}

ExitHandler_mac::~ExitHandler_mac()
{
    this_ = NULL;
}
