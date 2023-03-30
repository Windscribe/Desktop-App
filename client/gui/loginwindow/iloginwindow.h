#ifndef ILOGINWINDOW_H
#define ILOGINWINDOW_H

#include <QString>
#include <QGraphicsObject>

// abstract interface for login window
class ILoginWindow
{
public:

    enum ERROR_MESSAGE_TYPE { ERR_MSG_EMPTY, ERR_MSG_NO_INTERNET_CONNECTIVITY, ERR_MSG_NO_API_CONNECTIVITY,
                              ERR_MSG_PROXY_REQUIRES_AUTH, ERR_MSG_INVALID_API_RESPONSE,
                              ERR_MSG_INVALID_API_ENDPOINT, ERR_MSG_INCORRECT_LOGIN1,
                              ERR_MSG_INCORRECT_LOGIN2, ERR_MSG_INCORRECT_LOGIN3,
                              ERR_MSG_SESSION_EXPIRED, ERR_MSG_ACCOUNT_DISABLED, ERR_MSG_RATE_LIMITED,
                              ERR_MSG_USERNAME_IS_EMAIL };

    virtual ~ILoginWindow() {}
    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void setErrorMessage(ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage) = 0; // if errorMessageType is ERR_MSG_EMPTY, then clear error
    virtual void setEmergencyConnectState(bool isEmergencyConnected) = 0;
    virtual void setClickable(bool enabled) = 0;
    virtual void setCurrent2FACode(QString code) = 0;
    virtual void resetState() = 0;
    virtual void transitionToUsernameScreen() = 0;
    virtual void setFirewallTurnOffButtonVisibility(bool visible) = 0;
    virtual void updateScaling() = 0;

signals:
    virtual void minimizeClick() = 0;
    virtual void closeClick() = 0;
    virtual void preferencesClick() = 0;
    virtual void emergencyConnectClick() = 0;
    virtual void externalConfigModeClick() = 0;
    virtual void twoFactorAuthClick(const QString &username, const QString &password) = 0;
    virtual void loginClick(const QString &username, const QString &password,
                            const QString &code2fa) = 0;
    virtual void haveAccountYesClick() = 0;
    virtual void backToWelcomeClick() = 0;
};

Q_DECLARE_INTERFACE(ILoginWindow, "ILoginWindow")

#endif // ILOGINWINDOW_H
