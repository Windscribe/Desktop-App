#include "utils.h"

#include <QString>

#include <iostream>
#include <cstring>

#ifdef Q_OS_LINUX
    #include <termios.h>
    #include <unistd.h>
#elif defined(Q_OS_MACOS)
    #include <readpassphrase.h>
#else
    #include <windows.h>
#endif

QString Utils::getInput(const QString &prompt, bool disableEcho)
{
#if defined(Q_OS_WIN)
    DWORD mode;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    if (disableEcho) {
        GetConsoleMode(hStdin, &mode);
        SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);
    }

    std::string input;
    std::cout << prompt.toStdString();
    std::cin >> input;

    if (disableEcho) {
        std::cout << std::endl;
        SetConsoleMode(hStdin, mode);
    }

    return QString::fromStdString(input);
#elif defined(Q_OS_MAC)
    char buf[1024];
    memset(buf, 0, 1024);

    char *ret = readpassphrase(prompt.toStdString().c_str(), buf, 1024, disableEcho ? RPP_ECHO_OFF : RPP_ECHO_ON);
    if (ret == NULL) {
        return "";
    }
    return ret;
#else
    struct termios tty;
    tcflag_t oldFlags;

    if (disableEcho) {
        tcgetattr(STDIN_FILENO, &tty);
        oldFlags = tty.c_lflag;
        tty.c_lflag &= ~ECHO;
        (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }

    std::cout << prompt.toStdString();
    std::string input;
    std::cin >> input;

    if (disableEcho) {
        std::cout << std::endl;
        tty.c_lflag = oldFlags;
        (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }

    return QString::fromStdString(input);
#endif
}
