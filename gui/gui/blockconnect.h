#ifndef BLOCKCONNECT_H
#define BLOCKCONNECT_H

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
    void setBlockedMultiAccount(const QString &originalUsername);
    void setBlockedBannedUser();
    void setNeedUpgrade();

private:
    enum BLOCK_CONNECT { CONNECT_NOT_BLOCKED, CONNECT_BLOCKED_EXCEED_TRAFFIC, CONNECT_BLOCKED_MULTI_ACCOUNT, CONNECT_BLOCKED_BANNED_USER};
    BLOCK_CONNECT blockConnect_;
    bool bNeedUpgrade_;
    QString originalUsername_;
};

#endif // BLOCKCONNECT_H
