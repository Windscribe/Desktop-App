#pragma once

#include <QDateTime>
#include <QObject>

class AccountInfo : public QObject
{
    Q_OBJECT
public:
    explicit AccountInfo(QObject *parent = nullptr);

    QString username() const;
    void setUsername(const QString &u);

    QString email() const;
    void setEmail(const QString &e);

    bool isNeedConfirmEmail() const;
    void setNeedConfirmEmail(bool bConfirm);

    qint64 plan() const;
    void setPlan(qint64 plan);

    QString expireDate() const;
    void setExpireDate(const QString &expireDate);

    bool isPremium() const;
    void setIsPremium(bool isPremium);

    qint64 trafficUsed() const;
    void setTrafficUsed(qint64 used);

    QString lastReset() const;
    void setLastReset(const QString &lastReset);

    qsizetype alcCount() const;
    void setAlcCount(qsizetype count);

signals:
    void usernameChanged(const QString &username);
    void emailChanged(const QString &email);
    void isNeedConfirmEmailChanged(bool bNeedConfirm);
    void planChanged(qint64 plan);
    void expireDateChanged(const QString &date);
    void authHashChanged(const QString &authHash);
    void isPremiumChanged(bool isPremium);
    void trafficUsedChanged(qint64 used);
    void lastResetChanged(const QString &date);
    void alcCountChanged(qint64 count);

private:
    QString username_;
    QString email_;     // can be empty
    bool isNeedConfirmEmail_;
    qint64 planBytes_;       // -1 -> unlimited
    QString expireDate_;
    bool isPremium_;
    qint64 trafficUsed_;
    QString lastReset_;
    qsizetype alcCount_;
};
