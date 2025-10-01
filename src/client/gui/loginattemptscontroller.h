#pragma once

#include <QObject>
#include "loginwindow/loginwindowtypes.h"

class LoginAttemptsController
{
public:
    LoginAttemptsController();

    void pushIncorrectLogin();
    void reset();
    int attempts();
    LoginWindow::ERROR_MESSAGE_TYPE currentMessage() const;

private:
    int loginAttempts_;

};
