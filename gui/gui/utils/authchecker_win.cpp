#include "authchecker_win.h"

#include "utils/winutils.h"

AuthChecker_win::AuthChecker_win(QObject *parent) : IAuthChecker(parent)
{

}

bool AuthChecker_win::authenticate()
{
    return WinUtils::authorizeWithUac();
}
