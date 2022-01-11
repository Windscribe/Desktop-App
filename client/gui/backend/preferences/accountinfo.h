#ifndef ACCOUNTINFO_H
#define ACCOUNTINFO_H

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

    QString authHash() const;
    void setAuthHash(const QString &authHash);

signals:
    void usernameChanged(const QString &username);
    void emailChanged(const QString &email);
    void isNeedConfirmEmailChanged(bool bNeedConfirm);
    void planChanged(qint64 plan);
    void expireDateChanged(const QString &date);
    void authHashChanged(const QString &authHash);

private:
    QString username_;
    QString email_;     // can be empty
    bool isNeedConfirmEmail_;
    qint64 planBytes_;       // -1 -> unlimited
    QString expireDate_;
    QString authHash_;
};

#endif // ACCOUNTINFO_H
