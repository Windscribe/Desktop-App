#include "blockconnect.h"
#include <QObject>
#include "utils/ws_assert.h"

BlockConnect::BlockConnect() : blockConnect_(CONNECT_NOT_BLOCKED), bNeedUpgrade_(false)
{
}

bool BlockConnect::isBlocked()
{
    return bNeedUpgrade_ || blockConnect_ != CONNECT_NOT_BLOCKED;
}

bool BlockConnect::isBlockedExceedTraffic() const
{
    return blockConnect_ == CONNECT_BLOCKED_EXCEED_TRAFFIC;
}

QString BlockConnect::message()
{
    if (bNeedUpgrade_) {
        return QObject::tr("Your application version is no longer supported. Please update to continue using Windscribe.");
    }

    if (blockConnect_ == CONNECT_NOT_BLOCKED) {
        return "";
    } else if (blockConnect_ == CONNECT_BLOCKED_EXCEED_TRAFFIC) {
        return QObject::tr("Please upgrade to a Pro account to continue using Windscribe.");
    } else if (blockConnect_ == CONNECT_BLOCKED_MULTI_ACCOUNT) {
        return QObject::tr("Your original account %1 has expired. Creating multiple accounts to bypass free tier limitations is prohibited. Please login into the original account and wait until the bandwidth is reset. You can also upgrade to Pro.").arg(originalUsername_);
    } else if (blockConnect_ == CONNECT_BLOCKED_BANNED_USER) {
        return QObject::tr("Your account is disabled for abuse.");
    } else {
        WS_ASSERT(false);
        return "Unknown message from BlockConnect";
    }
}

void BlockConnect::setNotBlocking()
{
    blockConnect_ = CONNECT_NOT_BLOCKED;
}

void BlockConnect::setBlockedExceedTraffic()
{
    blockConnect_ = CONNECT_BLOCKED_EXCEED_TRAFFIC;
}

void BlockConnect::setBlockedMultiAccount(const QString &originalUsername, const QString &originalUserId)
{
    blockConnect_ = CONNECT_BLOCKED_MULTI_ACCOUNT;
    originalUsername_ = originalUsername;
}

void BlockConnect::setBlockedBannedUser()
{
    blockConnect_ = CONNECT_BLOCKED_BANNED_USER;
}

void BlockConnect::setNeedUpgrade()
{
    bNeedUpgrade_ = true;
}
