#pragma once

#include <QString>

class BlockConnect
{
public:
    BlockConnect();

    bool isBlocked();
    bool isBlockedExceedTraffic() const;
    QString message();
    void setNotBlocking();
    void setBlockedExceedTraffic();
    void setBlockedMultiAccount(const QString &originalUsername, const QString &originalUserId);
    void setBlockedBannedUser();
    void setNeedUpgrade();

private:
    enum BLOCK_CONNECT { CONNECT_NOT_BLOCKED, CONNECT_BLOCKED_EXCEED_TRAFFIC, CONNECT_BLOCKED_MULTI_ACCOUNT, CONNECT_BLOCKED_BANNED_USER};
    BLOCK_CONNECT blockConnect_;
    bool bNeedUpgrade_;
    QString originalUsername_;
};
