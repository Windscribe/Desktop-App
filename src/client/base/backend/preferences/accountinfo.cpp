#include "accountinfo.h"

AccountInfo::AccountInfo(QObject *parent) : QObject(parent)
{
    isNeedConfirmEmail_ = true;
    planBytes_ = -1;
    isPremium_ = false;
}

QString AccountInfo::username() const
{
    return username_;
}

void AccountInfo::setUsername(const QString &u)
{
    if (username_ != u)
    {
        username_ = u;
        emit usernameChanged(username_);
    }
}

QString AccountInfo::email() const
{
    return email_;
}

void AccountInfo::setEmail(const QString &e)
{
    if (email_ != e)
    {
        email_ = e;
        emit emailChanged(email_);
    }
}

bool AccountInfo::isNeedConfirmEmail() const
{
    return isNeedConfirmEmail_;
}

void AccountInfo::setNeedConfirmEmail(bool bConfirm)
{
    if (bConfirm != isNeedConfirmEmail_)
    {
        isNeedConfirmEmail_ = bConfirm;
        emit isNeedConfirmEmailChanged(isNeedConfirmEmail_);
    }
}

qint64 AccountInfo::plan() const
{
    return planBytes_;
}

void AccountInfo::setPlan(qint64 plan)
{
    if (plan != planBytes_)
    {
        planBytes_ = plan;
        emit planChanged(planBytes_);
    }
}

qint64 AccountInfo::trafficUsed() const
{
    return trafficUsed_;
}

void AccountInfo::setTrafficUsed(qint64 used)
{
    if (trafficUsed_ != used)
    {
        trafficUsed_ = used;
        emit trafficUsedChanged(trafficUsed_);
    }
}

QString AccountInfo::expireDate() const
{
    return expireDate_;
}

void AccountInfo::setExpireDate(const QString &resetDate)
{
    if (resetDate != expireDate_)
    {
        expireDate_ = resetDate;
        emit expireDateChanged(expireDate_);
    }
}

QString AccountInfo::lastReset() const
{
    return lastReset_;
}

void AccountInfo::setLastReset(const QString &lastReset)
{
    if (lastReset_ != lastReset)
    {
        lastReset_ = lastReset;
        emit lastResetChanged(lastReset_);
    }
}

bool AccountInfo::isPremium() const
{
    return isPremium_;
}

void AccountInfo::setIsPremium(bool isPremium)
{
    if (isPremium != isPremium_)
    {
        isPremium_ = isPremium;
        emit isPremiumChanged(isPremium_);
    }
}

qsizetype AccountInfo::alcCount() const
{
    return alcCount_;
}

void AccountInfo::setAlcCount(qsizetype count)
{
    if (alcCount_ != count) {
        alcCount_ = count;
        emit alcCountChanged(alcCount_);
    }
}
