#include "loginattemptscontroller.h"

LoginAttemptsController::LoginAttemptsController() : loginAttempts_(0)
{

}

void LoginAttemptsController::pushIncorrectLogin()
{
    loginAttempts_++;
}

void LoginAttemptsController::reset()
{
    loginAttempts_ = 0;
}

ILoginWindow::ERROR_MESSAGE_TYPE LoginAttemptsController::currentMessage() const
{
    if (loginAttempts_ == 1)
    {
        return ILoginWindow::ERR_MSG_INCORRECT_LOGIN1;
    }
    else if (loginAttempts_ == 2)
    {
        return ILoginWindow::ERR_MSG_INCORRECT_LOGIN2;
    }
    else if (loginAttempts_ > 2)
    {
        return ILoginWindow::ERR_MSG_INCORRECT_LOGIN3;
    }
    return ILoginWindow::ERR_MSG_INCORRECT_LOGIN1;
}
