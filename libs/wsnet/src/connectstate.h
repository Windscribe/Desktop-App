#pragma once
#include <mutex>
#include <map>

namespace wsnet {

typedef std::function<void(bool isConnected)> ConnectedToVpnStateChangedCallback;

// Provides Network and VPN connection state
// Thread safe
class ConnectState
{
public:
    void setConnectivityState(bool isOnline)
    {
        std::lock_guard locker(mutex_);
        isOnline_ = isOnline;
    }

    void setIsConnectedToVpnState(bool isConnected)
    {
        std::lock_guard locker(mutex_);
        if (isVPNConnected_ != isConnected) {
            isVPNConnected_ = isConnected;
            // notify subscribers
            for (const auto &it : subscribers_) {
                it.second(isVPNConnected_);
            }
        }
    }

    bool isOnline() const
    {
        std::lock_guard locker(mutex_);
        return isOnline_;
    }

    bool isVPNConnected() const
    {
        std::lock_guard locker(mutex_);
        return isVPNConnected_;
    }

    // returns id which must be used in the unsubscribeConnectedToVpnState(id) function
    std::uint32_t subscribeConnectedToVpnState(ConnectedToVpnStateChangedCallback callback)
    {
        std::lock_guard locker(mutex_);
        std::uint32_t id = curId_;
        curId_++;
        subscribers_[id] = callback;
        return id;
    }

    void unsubscribeConnectedToVpnState(std::uint32_t id)
    {
        std::lock_guard locker(mutex_);
        subscribers_.erase(id);
    }

private:
    mutable std::mutex mutex_;
    bool isOnline_ = true;
    bool isVPNConnected_ = false;
    std::uint32_t curId_ = 0;
    std::map<std::uint32_t, ConnectedToVpnStateChangedCallback> subscribers_;
};

} // namespace wsnet
