#include "splittunnelextensionmanager.test.h"

#import <Foundation/Foundation.h>
#import <SystemExtensions/SystemExtensions.h>
#include <QSignalSpy>
#include <QtTest>

#include "splittunnelextension/splittunnelextensionstate_mac.h"
#include "splittunnelextension/systemextensions_mac.h"

@interface FakeProxySession : NSObject
@property NEVPNStatus status;
@property NSUInteger starts;
@property BOOL refuseStart;
@end

@implementation FakeProxySession
- (BOOL)startTunnelWithOptions:(NSDictionary *)options andReturnError:(NSError **)error
{
    self.starts++;
    if (self.refuseStart) {
        *error = [NSError errorWithDomain:@"test" code:1 userInfo:nil];
        return NO;
    }
    self.status = NEVPNStatusConnected;
    return YES;
}
- (void)stopTunnel
{
    self.status = NEVPNStatusDisconnected;
}
@end

@interface FakeProxyManager : NSObject
@property(strong) FakeProxySession *connection;
@end

@implementation FakeProxyManager
@end

@interface FakeExtensionProperties : NSObject
@property(strong) NSString *bundleIdentifier;
@property(strong) NSString *bundleVersion;
@property(getter=isEnabled) BOOL enabled;
@property(getter=isAwaitingUserApproval) BOOL awaitingUserApproval;
@end

@implementation FakeExtensionProperties
@end

static void drainMainQueue()
{
    bool done = false;
    bool *donePtr = &done;
    dispatch_async(dispatch_get_main_queue(), ^{ *donePtr = true; });
    QDeadlineTimer deadline(1000);
    while (!done && !deadline.hasExpired()) {
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    }
    QVERIFY(done);
}

void TestSplitTunnelExtensionManager::sessionFailureWaitsForReconnect()
{
    SplitTunnelExtensionManager manager([](SystemExtensions_mac::StateCallback callback) {
        callback(SystemExtensions_mac::Active);
    });
    FakeProxySession *session = [FakeProxySession new];
    session.status = NEVPNStatusDisconnected;
    FakeProxyManager *proxy = [FakeProxyManager new];
    proxy.connection = session;
    manager.state_->proxyManager = (NETransparentProxyManager *)proxy;
    manager.state_->startIssued = true;
    manager.state_->wantStarted = true;
    manager.isActive_ = true;
    SystemExtensions_mac::instance()->onExtensionStateChanged(SystemExtensions_mac::Active);
    QSignalSpy failures(&manager, &SplitTunnelExtensionManager::startFailed);

    manager.confirmSessionEnded((__bridge void *)session);
    QCOMPARE(failures.count(), 1);
    QCOMPARE(failures.at(0).at(0).value<SPLIT_TUNNEL_START_FAIL_REASON>(), SPLIT_TUNNEL_START_FAIL_REASON_MAC_SESSION_ENDED);
    QVERIFY(manager.isActive());
    QVERIFY(manager.state_->sessionFailed);

    manager.confirmSessionEnded((__bridge void *)session);
    manager.startExtension("en0", "utun1");
    manager.setSplitTunnelSettings(true, true, {}, {"192.0.2.1"}, {});
    drainMainQueue();
    QCOMPARE(session.starts, NSUInteger(0));
    QCOMPARE(failures.count(), 1);

    manager.setSplitTunnelSettings(false, true, {}, {"192.0.2.1"}, {});
    manager.stopExtension();
    manager.setSplitTunnelSettings(true, true, {}, {"192.0.2.1"}, {});
    manager.startExtension("en0", "utun1");
    drainMainQueue();
    QCOMPARE(session.starts, NSUInteger(1));
    QVERIFY(!manager.state_->sessionFailed);
    QVERIFY(manager.isActive());
    QCOMPARE(failures.count(), 1);

    session.status = NEVPNStatusDisconnected;
    manager.confirmSessionEnded((__bridge void *)session);
    QCOMPARE(failures.count(), 2);
}

void TestSplitTunnelExtensionManager::startFailureUsesGenericReason()
{
    SplitTunnelExtensionManager manager;
    FakeProxySession *session = [FakeProxySession new];
    session.status = NEVPNStatusDisconnected;
    session.refuseStart = YES;
    FakeProxyManager *proxy = [FakeProxyManager new];
    proxy.connection = session;
    manager.state_->proxyManager = (NETransparentProxyManager *)proxy;
    manager.isActive_ = true;
    SystemExtensions_mac::instance()->onExtensionStateChanged(SystemExtensions_mac::Active);
    QSignalSpy failures(&manager, &SplitTunnelExtensionManager::startFailed);

    manager.startExtension("en0", "utun1");
    drainMainQueue();
    QCOMPARE(failures.count(), 1);
    QCOMPARE(failures.at(0).at(0).value<SPLIT_TUNNEL_START_FAIL_REASON>(), SPLIT_TUNNEL_START_FAIL_REASON_DEFAULT);
    QVERIFY(!manager.state_->sessionFailed);
}

void TestSplitTunnelExtensionManager::recoveredOrReplacedSessionIsIgnored()
{
    SplitTunnelExtensionManager manager;
    FakeProxySession *session = [FakeProxySession new];
    session.status = NEVPNStatusConnected;
    FakeProxyManager *proxy = [FakeProxyManager new];
    proxy.connection = session;
    manager.state_->proxyManager = (NETransparentProxyManager *)proxy;
    manager.state_->startIssued = true;
    QSignalSpy failures(&manager, &SplitTunnelExtensionManager::startFailed);

    manager.confirmSessionEnded((__bridge void *)session);
    QVERIFY(!manager.state_->sessionFailed);
    FakeProxySession *oldSession = [FakeProxySession new];
    oldSession.status = NEVPNStatusDisconnected;
    manager.confirmSessionEnded((__bridge void *)oldSession);
    QVERIFY(!manager.state_->sessionFailed);
    QCOMPARE(failures.count(), 0);
}

void TestSplitTunnelExtensionManager::freshStateClassifiesSessionFailure_data()
{
    QTest::addColumn<int>("state");
    QTest::addColumn<int>("status");
    QTest::addColumn<int>("reason");
    for (auto status : {NEVPNStatusDisconnected, NEVPNStatusInvalid}) {
        for (auto state : {SystemExtensions_mac::Active, SystemExtensions_mac::Inactive,
                           SystemExtensions_mac::PendingUserApproval, SystemExtensions_mac::Unknown}) {
            const bool disabled = state == SystemExtensions_mac::Inactive;
            const QByteArray name = QByteArray::number(status) + "-" + QByteArray::number(state);
            QTest::newRow(name.constData()) << int(state) << int(status)
                << int(disabled ? SPLIT_TUNNEL_START_FAIL_REASON_MAC_EXTENSION_NOT_ENABLED
                                : SPLIT_TUNNEL_START_FAIL_REASON_MAC_SESSION_ENDED);
        }
    }
}

void TestSplitTunnelExtensionManager::freshStateClassifiesSessionFailure()
{
    QFETCH(int, state);
    QFETCH(int, status);
    QFETCH(int, reason);
    SystemExtensions_mac::StateCallback reply;
    int queries = 0;
    SplitTunnelExtensionManager manager([&](SystemExtensions_mac::StateCallback callback) {
        ++queries;
        reply = callback;
    });
    FakeProxySession *session = [FakeProxySession new];
    session.status = NEVPNStatus(status);
    FakeProxyManager *proxy = [FakeProxyManager new];
    proxy.connection = session;
    manager.state_->proxyManager = (NETransparentProxyManager *)proxy;
    manager.state_->startIssued = true;
    manager.isActive_ = true;
    SystemExtensions_mac::instance()->onExtensionStateChanged(SystemExtensions_mac::Active);
    QSignalSpy failures(&manager, &SplitTunnelExtensionManager::startFailed);

    manager.confirmSessionEnded((__bridge void *)session);
    QCOMPARE(queries, 1);
    QVERIFY(reply);
    QCOMPARE(failures.count(), 0);
    manager.confirmSessionEnded((__bridge void *)session);
    QCOMPARE(queries, 1);
    reply(SystemExtensions_mac::SystemExtensionState(state));
    QCOMPARE(failures.count(), 1);
    QCOMPARE(int(failures.at(0).at(0).value<SPLIT_TUNNEL_START_FAIL_REASON>()), reason);
}

void TestSplitTunnelExtensionManager::disableBeforeSessionFailure()
{
    int queries = 0;
    SplitTunnelExtensionManager manager([&](SystemExtensions_mac::StateCallback) { ++queries; });
    FakeProxySession *session = [FakeProxySession new];
    session.status = NEVPNStatusDisconnected;
    FakeProxyManager *proxy = [FakeProxyManager new];
    proxy.connection = session;
    manager.state_->proxyManager = (NETransparentProxyManager *)proxy;
    manager.state_->startIssued = true;
    manager.isActive_ = true;
    QSignalSpy failures(&manager, &SplitTunnelExtensionManager::startFailed);

    SystemExtensions_mac::instance()->onExtensionStateChanged(SystemExtensions_mac::Inactive);
    manager.resetManager();
    drainMainQueue();
    manager.confirmSessionEnded((__bridge void *)session);
    QCOMPARE(queries, 0);
    QCOMPARE(failures.count(), 0);
}

void TestSplitTunnelExtensionManager::lateQueryReplyAfterStopIsIgnored_data()
{
    QTest::addColumn<bool>("reconnect");
    QTest::newRow("disabled-while-query-pending") << false;
    QTest::newRow("reconnected-while-query-pending") << true;
}

void TestSplitTunnelExtensionManager::lateQueryReplyAfterStopIsIgnored()
{
    QFETCH(bool, reconnect);
    SystemExtensions_mac::StateCallback reply;
    SplitTunnelExtensionManager manager([&](SystemExtensions_mac::StateCallback callback) { reply = callback; });
    FakeProxySession *session = [FakeProxySession new];
    session.status = NEVPNStatusDisconnected;
    FakeProxyManager *proxy = [FakeProxyManager new];
    proxy.connection = session;
    manager.state_->proxyManager = (NETransparentProxyManager *)proxy;
    manager.state_->startIssued = true;
    manager.isActive_ = true;
    QSignalSpy failures(&manager, &SplitTunnelExtensionManager::startFailed);

    manager.confirmSessionEnded((__bridge void *)session);
    QVERIFY(reply);
    if (reconnect) {
        manager.stopExtension();
        manager.startExtension("en0", "utun1");
    } else {
        manager.resetManager();
    }
    drainMainQueue();
    reply(SystemExtensions_mac::Active);
    QCOMPARE(failures.count(), 0);
    QCOMPARE(session.starts, NSUInteger(reconnect ? 1 : 0));
}

void TestSplitTunnelExtensionManager::failedFreshQueryDoesNotReuseCachedActive_data()
{
    QTest::addColumn<bool>("failed");
    QTest::newRow("request-failed") << true;
    QTest::newRow("completed-without-properties") << false;
}

void TestSplitTunnelExtensionManager::failedFreshQueryDoesNotReuseCachedActive()
{
    QFETCH(bool, failed);
    SystemExtensions_mac::instance()->onExtensionStateChanged(SystemExtensions_mac::Active);
    id delegate = [NSClassFromString(@"SystemExtensionRequestDelegate") new];
    QVERIFY(delegate);
    [delegate setValue:@YES forKey:@"isPropertiesRequest"];
    [delegate setValue:@YES forKey:@"requireFreshState"];
    __block int result = -1;
    [delegate setValue:^(SystemExtensions_mac::SystemExtensionState state) { result = int(state); } forKey:@"onState"];
    OSSystemExtensionRequest *request = [OSSystemExtensionRequest propertiesRequestForExtension:@"test" queue:dispatch_get_main_queue()];
    if (failed) {
        [delegate request:request didFailWithError:[NSError errorWithDomain:@"test" code:1 userInfo:nil]];
    } else {
        [delegate request:request didFinishWithResult:OSSystemExtensionRequestCompleted];
    }
    QCOMPARE(result, int(SystemExtensions_mac::Unknown));
    QCOMPARE(SystemExtensions_mac::instance()->lastKnownState(), SystemExtensions_mac::Active);
    [delegate request:request didFinishWithResult:OSSystemExtensionRequestCompleted];
    QCOMPARE(result, int(SystemExtensions_mac::Unknown));
    drainMainQueue();
}

void TestSplitTunnelExtensionManager::freshPropertiesOverrideCache_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::newRow("now-disabled") << false;
    QTest::newRow("now-enabled") << true;
}

void TestSplitTunnelExtensionManager::freshPropertiesOverrideCache()
{
    QFETCH(bool, enabled);
    SystemExtensions_mac::instance()->onExtensionStateChanged(enabled ? SystemExtensions_mac::Inactive : SystemExtensions_mac::Active);
    id delegate = [NSClassFromString(@"SystemExtensionRequestDelegate") new];
    QVERIFY(delegate);
    [delegate setValue:@YES forKey:@"isPropertiesRequest"];
    [delegate setValue:@YES forKey:@"requireFreshState"];
    __block int result = -1;
    [delegate setValue:^(SystemExtensions_mac::SystemExtensionState state) { result = int(state); } forKey:@"onState"];
    FakeExtensionProperties *properties = [FakeExtensionProperties new];
    properties.bundleIdentifier = @WS_MAC_SPLIT_TUNNEL_BUNDLE_ID;
    properties.bundleVersion = @"1";
    properties.enabled = enabled;
    QSignalSpy changes(SystemExtensions_mac::instance(), &SystemExtensions_mac::stateChanged);
    SystemExtensions_mac::instance()->setEnabledVersionMismatch(false);
    OSSystemExtensionRequest *request = [OSSystemExtensionRequest propertiesRequestForExtension:@"test" queue:dispatch_get_main_queue()];
    [delegate request:request foundProperties:(NSArray *)@[properties]];
    QCOMPARE(result, int(enabled ? SystemExtensions_mac::Active : SystemExtensions_mac::Inactive));
    QCOMPARE(int(SystemExtensions_mac::instance()->lastKnownState()), result);
    QCOMPARE(changes.count(), 1);
    if (enabled) {
        QVERIFY(SystemExtensions_mac::instance()->isEnabledVersionMismatch());
    }
    drainMainQueue();
}

void TestSplitTunnelExtensionManager::freshQueryDuringActivation()
{
    auto *system = SystemExtensions_mac::instance();
    system->activationInFlight_ = true;
    int result = -1;
    QSignalSpy changes(system, &SystemExtensions_mac::stateChanged);
    SystemExtensions_mac::queryFreshState([&](auto state) { result = int(state); });
    drainMainQueue();
    system->activationInFlight_ = false;
    QCOMPARE(result, int(SystemExtensions_mac::Unknown));
    QCOMPARE(changes.count(), 0);
}

void TestSplitTunnelExtensionManager::freshReplyAcrossActivation_data()
{
    QTest::addColumn<bool>("inFlight");
    QTest::newRow("activation-still-running") << true;
    QTest::newRow("activation-already-finished") << false;
}

void TestSplitTunnelExtensionManager::freshReplyAcrossActivation()
{
    QFETCH(bool, inFlight);
    auto *system = SystemExtensions_mac::instance();
    system->onExtensionStateChanged(SystemExtensions_mac::Active);
    system->setEnabledVersionMismatch(false);
    id delegate = [NSClassFromString(@"SystemExtensionRequestDelegate") new];
    [delegate setValue:@YES forKey:@"isPropertiesRequest"];
    [delegate setValue:@YES forKey:@"requireFreshState"];
    [delegate setValue:@(system->activationGeneration_) forKey:@"activationGeneration"];
    ++system->activationGeneration_;
    system->activationInFlight_ = inFlight;
    __block int result = -1;
    [delegate setValue:^(SystemExtensions_mac::SystemExtensionState state) { result = int(state); } forKey:@"onState"];
    FakeExtensionProperties *properties = [FakeExtensionProperties new];
    properties.bundleIdentifier = @WS_MAC_SPLIT_TUNNEL_BUNDLE_ID;
    properties.bundleVersion = @"1";
    properties.enabled = YES;
    QSignalSpy changes(system, &SystemExtensions_mac::stateChanged);
    OSSystemExtensionRequest *request = [OSSystemExtensionRequest propertiesRequestForExtension:@"test" queue:dispatch_get_main_queue()];
    [delegate request:request foundProperties:(NSArray *)@[properties]];
    system->activationInFlight_ = false;
    QCOMPARE(result, int(SystemExtensions_mac::Unknown));
    QCOMPARE(changes.count(), 0);
    QCOMPARE(system->lastKnownState(), SystemExtensions_mac::Active);
    QVERIFY(!system->isEnabledVersionMismatch());
    drainMainQueue();
}

QTEST_GUILESS_MAIN(TestSplitTunnelExtensionManager)
