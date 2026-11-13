#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <NetworkExtension/NetworkExtension.h>

#include <spdlog/spdlog.h>
#include <string>
#include <sys/resource.h>
#include <thread>

#import "../FlowUDP.h"
#include "testhelpers.h"

using namespace testhelpers;

typedef void (^ReadHandler)(NSArray<NSData *> *_Nullable, NSArray<NWEndpoint *> *_Nullable, NSError *_Nullable);
typedef void (^WriteCompletion)(NSError *_Nullable);

typedef NS_ENUM(NSInteger, FakeWriteMode) {
    FakeWriteModeSucceed = 0,
    FakeWriteModeHold,
    FakeWriteModeFail,
};

@interface FakeFlow : NSObject <SplitTunnelUDPFlow>
@property (nonatomic, nullable) nw_interface_t networkInterface;
@property (atomic) FakeWriteMode writeMode;
@property (atomic) BOOL closedRead;
@property (atomic) BOOL closedWrite;
@end

@implementation FakeFlow {
    ReadHandler pendingRead_;
    NSMutableArray<NSData *> *written_;
    NSMutableArray<NWEndpoint *> *writtenFrom_;
    NSMutableArray<WriteCompletion> *heldWrites_;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        written_ = [NSMutableArray array];
        writtenFrom_ = [NSMutableArray array];
        heldWrites_ = [NSMutableArray array];
    }
    return self;
}

- (NWEndpoint *)localEndpoint {
    return nil;
}

- (NEFlowMetaData *)metaData {
    return nil;
}

- (void)openWithLocalEndpoint:(NWHostEndpoint *)localEndpoint completionHandler:(void (^)(NSError *))completionHandler {
    completionHandler(nil);
}

- (void)readDatagramsWithCompletionHandler:(ReadHandler)completionHandler {
    @synchronized (self) {
        pendingRead_ = completionHandler;
    }
}

- (void)writeDatagrams:(NSArray<NSData *> *)datagrams
       sentByEndpoints:(NSArray<NWEndpoint *> *)remoteEndpoints
     completionHandler:(WriteCompletion)completionHandler {
    FakeWriteMode mode;
    @synchronized (self) {
        [written_ addObjectsFromArray:datagrams];
        [writtenFrom_ addObjectsFromArray:remoteEndpoints];
        mode = self.writeMode;
        if (mode == FakeWriteModeHold) {
            [heldWrites_ addObject:completionHandler];
        }
    }
    if (mode == FakeWriteModeSucceed) {
        completionHandler(nil);
    } else if (mode == FakeWriteModeFail) {
        completionHandler([NSError errorWithDomain:@"flowudp.test" code:1 userInfo:nil]);
    }
}

- (void)closeReadWithError:(NSError *)error {
    self.closedRead = YES;
}

- (void)closeWriteWithError:(NSError *)error {
    self.closedWrite = YES;
}

- (BOOL)hasPendingRead {
    @synchronized (self) {
        return pendingRead_ != nil;
    }
}

- (BOOL)deliver:(NSArray<NSData *> *)datagrams endpoints:(NSArray<NWEndpoint *> *)endpoints error:(NSError *)error {
    if (!waitUntil([self] { return [self hasPendingRead]; })) {
        return NO;
    }
    ReadHandler handler;
    @synchronized (self) {
        handler = pendingRead_;
        pendingRead_ = nil;
    }
    @autoreleasepool {
        handler(datagrams, endpoints, error);
    }
    return YES;
}

- (NSUInteger)writtenCount {
    @synchronized (self) {
        return written_.count;
    }
}

- (NSString *)writtenPayloadAt:(NSUInteger)index {
    @synchronized (self) {
        return [[NSString alloc] initWithData:written_[index] encoding:NSUTF8StringEncoding];
    }
}

- (NWHostEndpoint *)writtenFromAt:(NSUInteger)index {
    @synchronized (self) {
        return (NWHostEndpoint *)writtenFrom_[index];
    }
}

- (void)releaseHeldWrites {
    NSArray<WriteCompletion> *held;
    @synchronized (self) {
        held = [heldWrites_ copy];
        [heldWrites_ removeAllObjects];
    }
    for (WriteCompletion completion in held) {
        completion(nil);
    }
}

- (BOOL)closed {
    return self.closedRead && self.closedWrite;
}

@end

@interface FakeRouting : NSObject <SplitTunnelUDPRouting>
@property (atomic) BOOL split;
@end

@implementation FakeRouting

- (BOOL)isSplitTunnelApplicable:(NEAppProxyFlow *)flow remoteEndpoint:(nw_endpoint_t)remoteEndpoint {
    return self.split;
}

- (BOOL)isVpnServer:(nw_endpoint_t)endpoint {
    return NO;
}

- (BOOL)isFromVpnProcess:(NEAppProxyFlow *)flow {
    return NO;
}

@end

namespace {

nw_interface_t g_loopback = nil;
FlowUDP *g_udp = nil;
FakeRouting *g_routing = nil;

nw_interface_t findLoopback()
{
    __block nw_interface_t found = nil;
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    nw_path_monitor_t monitor = nw_path_monitor_create_with_type(nw_interface_type_loopback);
    nw_path_monitor_set_queue(monitor, dispatch_queue_create("flowudp.test.monitor", DISPATCH_QUEUE_SERIAL));
    nw_path_monitor_set_update_handler(monitor, ^(nw_path_t path) {
        nw_path_enumerate_interfaces(path, ^bool(nw_interface_t interface) {
            if (!found && nw_interface_get_type(interface) == nw_interface_type_loopback) {
                found = interface;
                dispatch_semaphore_signal(semaphore);
                return false;
            }
            return true;
        });
    });
    nw_path_monitor_start(monitor);
    dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC));
    nw_path_monitor_cancel(monitor);
    return found;
}

NSData *data(const std::string &payload)
{
    return [NSData dataWithBytes:payload.data() length:payload.size()];
}

NWHostEndpoint *endpoint(const std::string &host, uint16_t port)
{
    return [NWHostEndpoint endpointWithHostname:[NSString stringWithUTF8String:host.c_str()]
                                           port:[NSString stringWithFormat:@"%u", port]];
}

NWHostEndpoint *endpoint(const Peer &peer)
{
    return endpoint(peer.host(), peer.port());
}

FakeFlow *startFlow(BOOL onTunnel = YES)
{
    FakeFlow *flow = [[FakeFlow alloc] init];
    flow.networkInterface = g_loopback;
    @autoreleasepool {
        VERIFY([g_udp setupUDPConnection:flow interface:g_loopback onTunnel:onTunnel]);
    }
    VERIFY(waitUntil([flow] { return [flow hasPendingRead]; }));
    return flow;
}

bool send(FakeFlow *flow, const Peer &peer, const std::string &payload)
{
    return [flow deliver:@[data(payload)] endpoints:@[endpoint(peer)] error:nil];
}

bool receives(Peer &peer, const std::string &expected, uint16_t *fromPort = nullptr)
{
    std::string payload;
    uint16_t port = 0;
    if (!peer.receive(payload, port)) {
        return false;
    }
    if (fromPort) {
        *fromPort = port;
    }
    return payload == expected;
}

void closeFlow(FakeFlow *flow)
{
    [flow deliver:@[] endpoints:@[] error:nil];
}

void testSendsShareOneSocketAndSourcePort(FakeFlow *flow, Peer peers[3], uint16_t &relayPort)
{
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    const BOOL delivered = [flow deliver:@[data("a0"), data("a1"), data("a2")]
                               endpoints:@[endpoint(peers[0]), endpoint(peers[1]), endpoint(peers[2])]
                                   error:nil];
    VERIFY(delivered);
    uint16_t ports[3] = {0, 0, 0};
    for (int i = 0; i < 3; ++i) {
        VERIFY(receives(peers[i], "a" + std::to_string(i), &ports[i]));
    }
    VERIFY(ports[0] != 0 && ports[0] == ports[1] && ports[1] == ports[2]);
    VERIFY_EQUAL(openFileCount(), before + 1);
    relayPort = ports[0];
}

void testRepliesReachTheFlow(FakeFlow *flow, Peer peers[3], uint16_t relayPort)
{
    const NSUInteger before = [flow writtenCount];
    VERIFY(peers[1].sendTo(relayPort, "reply"));
    VERIFY(waitUntil([&] { return [flow writtenCount] == before + 1; }));
    if ([flow writtenCount] == before + 1) {
        VERIFY([[flow writtenPayloadAt:before] isEqualToString:@"reply"]);
        NWHostEndpoint *from = [flow writtenFromAt:before];
        VERIFY([from.hostname isEqualToString:@"127.0.0.1"]);
        VERIFY(from.port.intValue == peers[1].port());
    }
}

void testUncontactedPeerReachesTheFlow(FakeFlow *flow, uint16_t relayPort)
{
    Peer stranger;
    const NSUInteger before = [flow writtenCount];
    VERIFY(stranger.sendTo(relayPort, "stranger"));
    VERIFY(waitUntil([&] { return [flow writtenCount] == before + 1; }));
    if ([flow writtenCount] == before + 1) {
        VERIFY([[flow writtenPayloadAt:before] isEqualToString:@"stranger"]);
        VERIFY([flow writtenFromAt:before].port.intValue == stranger.port());
    }
}

void testManyDestinationsUseNoExtraFiles(FakeFlow *flow, Peer &peer)
{
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    for (uint16_t base = 41000; base < 43000; base += 100) {
        NSMutableArray<NSData *> *datagrams = [NSMutableArray array];
        NSMutableArray<NWEndpoint *> *endpoints = [NSMutableArray array];
        for (uint16_t port = base; port < base + 100; ++port) {
            [datagrams addObject:data("x")];
            [endpoints addObject:endpoint("127.0.0.1", port)];
        }
        VERIFY([flow deliver:datagrams endpoints:endpoints error:nil]);
    }
    VERIFY([flow hasPendingRead] || waitUntil([flow] { return [flow hasPendingRead]; }));
    VERIFY_EQUAL(openFileCount(), before);
    VERIFY(send(flow, peer, "after-many"));
    VERIFY(receives(peer, "after-many"));
    VERIFY(![flow closed]);
}

void testIpv6AddsOneSocket(FakeFlow *flow)
{
    Peer v6(AF_INET6);
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    uint16_t relayPort = 0;
    VERIFY(send(flow, v6, "v6"));
    VERIFY(receives(v6, "v6", &relayPort));
    VERIFY_EQUAL(openFileCount(), before + 1);
    VERIFY(send(flow, v6, "v6-again"));
    VERIFY(receives(v6, "v6-again"));
    VERIFY_EQUAL(openFileCount(), before + 1);

    const NSUInteger written = [flow writtenCount];
    VERIFY(v6.sendTo(relayPort, "v6-reply"));
    VERIFY(waitUntil([&] { return [flow writtenCount] == written + 1; }));
    if ([flow writtenCount] == written + 1) {
        VERIFY([[flow writtenPayloadAt:written] isEqualToString:@"v6-reply"]);
        NWHostEndpoint *from = [flow writtenFromAt:written];
        VERIFY([from.hostname isEqualToString:@"::1"]);
        VERIFY(from.port.intValue == v6.port());
    }
}

void testWritesWaitForCompletion(FakeFlow *flow, Peer &peer, uint16_t relayPort)
{
    flow.writeMode = FakeWriteModeHold;
    const NSUInteger before = [flow writtenCount];
    VERIFY(peer.sendTo(relayPort, "held-1"));
    VERIFY(waitUntil([&] { return [flow writtenCount] == before + 1; }));
    VERIFY(peer.sendTo(relayPort, "held-2"));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    VERIFY([flow writtenCount] == before + 1);
    flow.writeMode = FakeWriteModeSucceed;
    [flow releaseHeldWrites];
    VERIFY(waitUntil([&] { return [flow writtenCount] == before + 2; }));
    if ([flow writtenCount] == before + 2) {
        VERIFY([[flow writtenPayloadAt:before + 1] isEqualToString:@"held-2"]);
    }
}

void testFailedWriteClosesFlowAndSockets()
{
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    FakeFlow *flow = startFlow();
    Peer peer;
    uint16_t relayPort = 0;
    VERIFY(send(flow, peer, "out"));
    VERIFY(receives(peer, "out", &relayPort));
    VERIFY_EQUAL(openFileCount(), before + 2);

    flow.writeMode = FakeWriteModeFail;
    VERIFY(peer.sendTo(relayPort, "in"));
    VERIFY(waitUntil([flow] { return [flow closed]; }));
    VERIFY(waitUntil([&] { return openFileCount() == before + 1; }));
}

void testAppCloseClosesSockets()
{
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    Peer peer;

    FakeFlow *errored = startFlow();
    VERIFY(send(errored, peer, "e"));
    VERIFY(receives(peer, "e"));
    VERIFY_EQUAL(openFileCount(), before + 2);
    VERIFY([errored deliver:nil endpoints:nil error:[NSError errorWithDomain:@"flowudp.test" code:2 userInfo:nil]]);
    VERIFY([errored closed]);
    VERIFY(waitUntil([&] { return openFileCount() == before + 1; }));

    FakeFlow *ended = startFlow();
    VERIFY(send(ended, peer, "f"));
    VERIFY(receives(peer, "f"));
    VERIFY_EQUAL(openFileCount(), before + 2);
    closeFlow(ended);
    VERIFY([ended closed]);
    VERIFY(waitUntil([&] { return openFileCount() == before + 1; }));
}

void testStopClosesEverything()
{
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    Peer peer;
    FakeFlow *first = startFlow();
    FakeFlow *second = startFlow();
    FakeFlow *idle = startFlow();
    VERIFY(send(first, peer, "1"));
    VERIFY(receives(peer, "1"));
    VERIFY(send(second, peer, "2"));
    VERIFY(receives(peer, "2"));
    VERIFY_EQUAL(openFileCount(), before + 3);

    @autoreleasepool {
        [g_udp cleanup];
    }
    VERIFY([first closed]);
    VERIFY([second closed]);
    VERIFY(![idle closed]);
    VERIFY(waitUntil([&] { return openFileCount() == before + 1; }));

    VERIFY(send(idle, peer, "after-stop"));
    VERIFY([idle closed]);
    std::string payload;
    uint16_t port = 0;
    VERIFY(!peer.receive(payload, port, 300));
    VERIFY_EQUAL(openFileCount(), before + 1);

    FakeFlow *late = [[FakeFlow alloc] init];
    VERIFY(![g_udp setupUDPConnection:late interface:g_loopback onTunnel:YES]);

    [g_udp setSettings:g_routing];
}

void testNonSplitOffTunnelIsDropped()
{
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    Peer peer;
    g_routing.split = NO;
    FakeFlow *flow = startFlow(NO);
    VERIFY(send(flow, peer, "dropped"));
    std::string payload;
    uint16_t port = 0;
    VERIFY(!peer.receive(payload, port, 300));
    VERIFY_EQUAL(openFileCount(), before + 1);
    VERIFY(![flow closed]);

    g_routing.split = YES;
    VERIFY(send(flow, peer, "split"));
    VERIFY(receives(peer, "split"));
    closeFlow(flow);
}

void testSocketsAreKeptPerInterface()
{
    Peer peer;
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    FakeFlow *flow = startFlow();
    flow.networkInterface = nil;

    uint16_t splitPort = 0;
    VERIFY(send(flow, peer, "split"));
    VERIFY(receives(peer, "split", &splitPort));

    g_routing.split = NO;
    uint16_t nonSplitPort = 0;
    VERIFY(send(flow, peer, "non-split"));
    VERIFY(receives(peer, "non-split", &nonSplitPort));
    g_routing.split = YES;

    VERIFY(splitPort != 0 && nonSplitPort != 0 && splitPort != nonSplitPort);
    VERIFY_EQUAL(openFileCount(), before + 2);
    closeFlow(flow);
}

void testNonAddressDestinationIsDropped()
{
    Peer peer;
    const int before = settledOpenFileCount(dispatch_get_main_queue());
    FakeFlow *flow = startFlow();
    const BOOL delivered = [flow deliver:@[data("name"), data("after-name")]
                               endpoints:@[endpoint("st-test.invalid", 9), endpoint(peer)]
                                   error:nil];
    VERIFY(delivered);
    VERIFY(receives(peer, "after-name"));
    VERIFY(waitUntil([flow] { return [flow hasPendingRead]; }));
    VERIFY_EQUAL(openFileCount(), before + 1);
    VERIFY(![flow closed]);
    closeFlow(flow);
}

void testOutOfFilesDropsDatagramAndRecovers()
{
    Peer peer;
    FakeFlow *flow = startFlow();

    struct rlimit original;
    VERIFY(getrlimit(RLIMIT_NOFILE, &original) == 0);
    const int lowestFree = dup(0);
    VERIFY(lowestFree >= 0);
    close(lowestFree);
    struct rlimit lowered = original;
    lowered.rlim_cur = static_cast<rlim_t>(lowestFree);
    VERIFY(setrlimit(RLIMIT_NOFILE, &lowered) == 0);

    VERIFY(send(flow, peer, "no-files"));
    VERIFY(setrlimit(RLIMIT_NOFILE, &original) == 0);
    std::string payload;
    uint16_t port = 0;
    VERIFY(!peer.receive(payload, port, 300));
    VERIFY(![flow closed]);

    VERIFY(send(flow, peer, "recovered"));
    VERIFY(receives(peer, "recovered"));
    closeFlow(flow);
}

void runTests()
{
    g_loopback = findLoopback();
    VERIFY(g_loopback != nil);

    g_routing = [[FakeRouting alloc] init];
    g_routing.split = YES;
    g_udp = [[FlowUDP alloc] init];
    [g_udp setSettings:g_routing];

    {
        FakeFlow *flow = startFlow();
        Peer peers[3];
        uint16_t relayPort = 0;
        testSendsShareOneSocketAndSourcePort(flow, peers, relayPort);
        testRepliesReachTheFlow(flow, peers, relayPort);
        testUncontactedPeerReachesTheFlow(flow, relayPort);
        testManyDestinationsUseNoExtraFiles(flow, peers[0]);
        testIpv6AddsOneSocket(flow);
        testWritesWaitForCompletion(flow, peers[2], relayPort);
        closeFlow(flow);
    }
    testFailedWriteClosesFlowAndSockets();
    testAppCloseClosesSockets();
    testStopClosesEverything();
    testNonSplitOffTunnelIsDropped();
    testSocketsAreKeptPerInterface();
    testNonAddressDestinationIsDropped();
    testOutOfFilesDropsDatagramAndRecovers();
}

}

int main()
{
    spdlog::set_level(spdlog::level::off);
    std::thread([] {
        @autoreleasepool {
            runTests();
        }
        if (g_failures == 0) {
            printf("PASS\n");
        }
        fflush(stdout);
        exit(g_failures);
    }).detach();
    dispatch_main();
}
