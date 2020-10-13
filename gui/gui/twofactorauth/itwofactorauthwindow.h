#ifndef ITWOFACTORAUTHWINDOW_H
#define ITWOFACTORAUTHWINDOW_H

#include <QGraphicsObject>

class ITwoFactorAuthWindow
{
public:
    enum ERROR_MESSAGE_TYPE { ERR_MSG_EMPTY, ERR_MSG_NO_CODE, ERR_MSG_INVALID_CODE };

    virtual ~ITwoFactorAuthWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void resetState() = 0;
    virtual void clearCurrentCredentials() = 0;
    virtual void setCurrentCredentials(const QString &username, const QString &password) = 0;
    virtual void setLoginMode(bool is_login_mode) = 0;
    virtual void setErrorMessage(ERROR_MESSAGE_TYPE errorMessage) = 0;
    virtual void setClickable(bool isClickable) = 0;
    virtual void updateScaling() = 0;

signals:
    virtual void addClick(const QString &code2fa) = 0;
    virtual void loginClick(const QString &username, const QString &password,
                            const QString &code2fa) = 0;
    virtual void escapeClick() = 0;
    virtual void minimizeClick() = 0;
    virtual void closeClick() = 0;
};

Q_DECLARE_INTERFACE(ITwoFactorAuthWindow, "ITwoFactorAuthWindow")


#endif // ITWOFACTORAUTHWINDOW_H
