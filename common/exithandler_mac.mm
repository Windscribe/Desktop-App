#include "exithandler_mac.h"
#include "Utils/logger.h"
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

    if (ExitHandler_mac::this_ && desc != nil)
    {
        NSAppleEventDescriptor *descQuitReason = [desc attributeDescriptorForKeyword:kAEQuitReason];
        if (descQuitReason != nil)
        {
            OSType eventType = [descQuitReason enumCodeValue];
            if (eventType == kAELogOut || eventType == kAEReallyLogOut)
            {
                qCDebug(LOG_BASIC) << "app close, reason: log out";
                ExitHandler_mac::this_->setExitWithRestart();
            }
            else if (eventType == kAEShowRestartDialog || eventType == kAERestart)
            {
                qCDebug(LOG_BASIC) << "app close, reason: system restart";
                ExitHandler_mac::this_->setExitWithRestart();
            }
            else if (eventType == kAEShowShutdownDialog || eventType == kAEShutDown)
            {
                qCDebug(LOG_BASIC) << "app close, reason: system shutdown";
                ExitHandler_mac::this_->setExitWithRestart();
            }
        }
    }

    emit ExitHandler_mac::this_->shouldTerminate();

    /*if (g_prevCloseImpl)
    {
        NSApplicationTerminateReply ret = ( (NSApplicationTerminateReply (*)(id, SEL)) (*g_prevCloseImpl))( self, sel_registerName("applicationShouldTerminate:"));
        return ret;
    }
    else
    {
        return NSTerminateNow;
    }*/
    return NSTerminateCancel;
}

ExitHandler_mac::ExitHandler_mac(QObject *parent) : QObject(parent), bExitWithRestart_(false)
{
    this_ = this;

    // TODO: fix objc_msgSend parameters
//    Class cls = objc_getClass("NSApplication");
//    id appInst = objc_msgSend((id)cls, sel_registerName("sharedApplication"));

//    if (appInst != NULL)
//    {
//        id delegate = objc_msgSend(appInst, sel_registerName("delegate"));
//        Class delClass = (Class)objc_msgSend(delegate,  sel_registerName("class"));
//        SEL exitHandle = sel_registerName("applicationShouldTerminate:");

//        Method mtdFunc = class_getInstanceMethod(delClass, exitHandle);
//        const char *types = method_getTypeEncoding(mtdFunc);

//        if (class_getInstanceMethod(delClass, exitHandle))
//        {
//            g_prevCloseImpl = class_replaceMethod(delClass, exitHandle, (IMP)exitMacHandler, types);
//        }
//        else
//        {
//            class_addMethod(delClass, exitHandle, (IMP)exitMacHandler, types);
//        }
//    }
}

ExitHandler_mac::~ExitHandler_mac()
{
    this_ = NULL;
}
