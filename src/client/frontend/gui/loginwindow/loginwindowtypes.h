#pragma once

#include <QString>
#include <QCoreApplication>

namespace LoginWindow {

enum ERROR_MESSAGE_TYPE { ERR_MSG_EMPTY, ERR_MSG_NO_INTERNET_CONNECTIVITY, ERR_MSG_NO_API_CONNECTIVITY,
                          ERR_MSG_PROXY_REQUIRES_AUTH, ERR_MSG_INVALID_API_RESPONSE,
                          ERR_MSG_INVALID_API_ENDPOINT, ERR_MSG_INCORRECT_LOGIN1,
                          ERR_MSG_INCORRECT_LOGIN2, ERR_MSG_INCORRECT_LOGIN3,
                          ERR_MSG_SESSION_EXPIRED, ERR_MSG_INVALID_SECURITY_TOKEN, ERR_MSG_ACCOUNT_DISABLED, ERR_MSG_RATE_LIMITED,
                          ERR_MSG_USERNAME_IS_EMAIL, ERR_MSG_SOME_ERROR };

struct ErrorMessageInfo {
    QString text;
    bool isError;
    
    ErrorMessageInfo() : isError(false) {}
    ErrorMessageInfo(const QString& t, bool e) : text(t), isError(e) {}
};

// Utility function to get error message text and error flag
inline ErrorMessageInfo getErrorMessageInfo(ERROR_MESSAGE_TYPE errorMessageType, const QString &customMessage = QString())
{
    switch(errorMessageType)
    {
        case ERR_MSG_EMPTY:
            return ErrorMessageInfo(QString(), false);
        case ERR_MSG_NO_INTERNET_CONNECTIVITY:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "No Internet Connectivity"), false);
        case ERR_MSG_NO_API_CONNECTIVITY:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "No API Connectivity"), false);
        case ERR_MSG_PROXY_REQUIRES_AUTH:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "Proxy requires authentication"), false);
        case ERR_MSG_INVALID_API_RESPONSE:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "Invalid API response, check your network"), false);
        case ERR_MSG_INVALID_API_ENDPOINT:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "Invalid API Endpoint"), false);
        case ERR_MSG_INCORRECT_LOGIN1:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "...hmm are you sure this is correct?"), true);
        case ERR_MSG_INCORRECT_LOGIN2:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "...Sorry, seems like it's wrong again"), true);
        case ERR_MSG_INCORRECT_LOGIN3:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "...hmm, try resetting your password!"), true);
        case ERR_MSG_RATE_LIMITED:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "Rate limited. Please wait before trying to login again."), true);
        case ERR_MSG_SESSION_EXPIRED:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "Session is expired. Please login again"), false);
        case ERR_MSG_USERNAME_IS_EMAIL:
            return ErrorMessageInfo(QCoreApplication::translate("LoginWindow::LoginWindowItem", "Your username should not be an email address. Please try again."), false);
        case ERR_MSG_ACCOUNT_DISABLED:
        case ERR_MSG_INVALID_SECURITY_TOKEN:
        case ERR_MSG_SOME_ERROR:
            return ErrorMessageInfo(customMessage, false);
        default:
            return ErrorMessageInfo(QString(), false);
    }
}

}
