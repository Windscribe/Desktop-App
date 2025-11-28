#include "pingmanager_apple.h"
#import "simple_ping.h"
#include "utils/utils.h"

// wrapper for SimplePing
// SimplePing it's Apple's recommended class for pinging: https://developer.apple.com/library/archive/samplecode/SimplePing
@interface PingHelper : NSObject <SimplePingDelegate>
@property (assign) std::function<void(int, std::uint64_t)> callback;
@property (assign) std::uint64_t userData;
@property (atomic, strong, readwrite, nullable) SimplePing *pinger;
@property(atomic, assign, nullable) NSTimer *timeoutTimer;
@property(atomic, assign) std::chrono::time_point<std::chrono::steady_clock> startTime;
- (void)ping:(NSString*)hostName data:(std::uint64_t)data;
@end

@implementation PingHelper
- (instancetype)initWithCallback: (std::function<void(int, std::uint64_t)>) callback
{
    self = [super init];
    if (self) {
        _callback = callback;
        _pinger = nil;
        _timeoutTimer = nil;
    }
    return self;
}

- (void)dealloc {
    [_pinger stop];
    [_timeoutTimer invalidate];
}


// pings the address
- (void)ping:(NSString*)hostName data: (std::uint64_t) data
{
    if (!hostName) {
        self.callback(-1, data);
        return;
    }
    self.userData = data;
    self.pinger = [[SimplePing alloc] initWithHostName: hostName];
    self.pinger.addressStyle = SimplePingAddressStyleICMPv4;
    self.pinger.delegate = self;
    [self.pinger start];
}

- (void)handleTimeout:(NSTimer *)timer {
    self.callback(-1, self.userData);
}

- (void)simplePing:(SimplePing *)pinger didStartWithAddress:(NSData *)address {
    // Send the first ping straight away.
    [self.pinger sendPingWithData:nil];
    self.startTime = std::chrono::steady_clock::now();
    // timeout for ping 2 sec
    self.timeoutTimer = [NSTimer scheduledTimerWithTimeInterval:2.0
                                               target:self
                                             selector:@selector(handleTimeout:)
                                             userInfo:nil
                                              repeats:NO];
}

- (void)simplePing:(SimplePing *)pinger didFailWithError:(NSError *)error {
    // No need to call -stop.  The pinger will stop itself in this case.
    self.callback(-1, self.userData);
}

- (void)simplePing:(SimplePing *)pinger didSendPacket:(NSData *)packet sequenceNumber:(uint16_t)sequenceNumber {
  // nothing todo
}

- (void)simplePing:(SimplePing *)pinger didFailToSendPacket:(NSData *)packet sequenceNumber:(uint16_t)sequenceNumber error:(NSError *)error {
    self.callback(-1, self.userData);
}

- (void)simplePing:(SimplePing *)pinger didReceivePingResponsePacket:(NSData *)packet sequenceNumber:(uint16_t)sequenceNumber {
    self.callback(utils::since(self.startTime).count(), self.userData);
}

- (void)simplePing:(SimplePing *)pinger didReceiveUnexpectedPacket:(NSData *)packet {
  // nothing todo
}

@end


namespace wsnet {

PingManager_apple::PingManager_apple()
{
    thread_ = std::thread(std::bind(&PingManager_apple::run, this));
    std::unique_lock<std::mutex> lock(mutex_);
    runLoopReady_.wait(lock, [this] { return isRunLoopReady_; });
}

PingManager_apple::~PingManager_apple()
{
    if (runLoop_) {
      stop();
    }
}

void PingManager_apple::ping(const std::string &hostname, PingFinishedCallbackFunc callback)
{
    std::uint64_t localCopyCurId = curId_++;
    void (^objcBlock)(void) = ^{
      // Need to make a copy, as capturing from a link is not safe
      std::string localCopyHostname = hostname;
      void *pingHelper = (__bridge_retained void*)[[PingHelper alloc] initWithCallback: std::bind(&PingManager_apple::callback, this, std::placeholders::_1, std::placeholders::_2)];
      {
          std::lock_guard<std::mutex> lock(mutex_);
          activePings_[localCopyCurId] = { pingHelper, callback};
      }
      NSString *ip_address = [NSString stringWithUTF8String:localCopyHostname.c_str()];
      [(__bridge PingHelper*)pingHelper ping: ip_address data: localCopyCurId];
    };
    // Schedule execution on RunLoop
    CFRunLoopPerformBlock(static_cast<CFRunLoopRef>(runLoop_),
                     kCFRunLoopDefaultMode,
                     (void (^)(void))objcBlock);
    CFRunLoopWakeUp(static_cast<CFRunLoopRef>(runLoop_));
}

void PingManager_apple::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &it : activePings_) {
            CFRelease(it.second.pingHelper);
        }
        activePings_.clear();
    }
    // Wake up the run loop to process the stop
    if (runLoop_) {
        CFRunLoopSourceSignal(static_cast<CFRunLoopSourceRef>(shutdownMsg_));
        CFRunLoopWakeUp(static_cast<CFRunLoopRef>(runLoop_));
    }

    thread_.join();
    runLoop_ = nullptr;
}

void PingManager_apple::receiveShutdownMsgCallback(void* info)
{
    // Calling CFRunLoopStop ouside the RunLoop thread may cause crashes
    // inside libpthread, so we need to stop the RunLoop here 
    PingManager_apple *this_ = static_cast<PingManager_apple *>(info);
    CFRunLoopStop(static_cast<CFRunLoopRef>(this_->runLoop_));
}

void PingManager_apple::run()
{
    runLoop_ = CFRunLoopGetCurrent();

    // Add a source to wake up RunLoop when it stops
    CFRunLoopSourceContext context;
    memset(&context, 0, sizeof(CFRunLoopSourceContext));
    context.info = this;
    context.perform = receiveShutdownMsgCallback;
    shutdownMsg_ = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    CFRunLoopAddSource(static_cast<CFRunLoopRef>(runLoop_), static_cast<CFRunLoopSourceRef>(shutdownMsg_), kCFRunLoopDefaultMode);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        isRunLoopReady_ = true;
    }
    runLoopReady_.notify_one();

    CFRunLoopRun();

    CFRunLoopRemoveSource(static_cast<CFRunLoopRef>(runLoop_), static_cast<CFRunLoopSourceRef>(shutdownMsg_), kCFRunLoopDefaultMode);
    CFRelease(static_cast<CFRunLoopSourceRef>(shutdownMsg_));
}

void PingManager_apple::callback(int timeMs, std::uint64_t id)
{
    PingFinishedCallbackFunc callbackFunc;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = activePings_.find(id);
        if (it == activePings_.end()) {
          return;
        }
        CFRelease(it->second.pingHelper);
        callbackFunc = it->second.callback;
        activePings_.erase(it);
    }
    callbackFunc(timeMs);
}


} // namespace wsnet
