#ifndef LOGINATTEMPTSCONTROLLER_H
#define LOGINATTEMPTSCONTROLLER_H

#include <QObject>
#include "loginwindow/iloginwindow.h"

class LoginAttemptsController
{
public:
    LoginAttemptsController();

    void pushIncorrectLogin();
    void reset();
    ILoginWindow::ERROR_MESSAGE_TYPE currentMessage() const;
    
private:
    int loginAttempts_;
    
};

#endif // LOGINATTEMPTSCONTROLLER_H
